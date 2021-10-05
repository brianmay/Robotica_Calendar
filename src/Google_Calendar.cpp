/*
   3-Google_calendar_example for e-radionica.com Inkplate 10
   For this example you will need only USB cable and Inkplate 10.
   Select "Inkplate 10(ESP32)" from Tools -> Board menu.
   Don't have "Inkplate 10(ESP32)" option? Follow our tutorial and add it:
   https://e-radionica.com/en/blog/add-inkplate-6-to-arduino-ide/

   This project shows you how Inkplate 10 can be used to display
   events in your Google Calendar using their provided API

   For this to work you need to change your timezone, wifi credentials and your private calendar url
   which you can find following these steps:

    1. Open your google calendar
    2. Click the 3 menu dots of the calendar you want to access at the bottom of left hand side
    3. Click 'Settings and sharing'
    4. Navigate to 'Integrate Calendar'
    5. Take the 'Secret address in iCal format'

   (https://support.google.com/calendar/thread/2408874?hl=en)

   Want to learn more about Inkplate? Visit www.inkplate.io
   Looking to get support? Write on our forums: http://forum.e-radionica.com/en/
   11 February 2021 by e-radionica.com
*/

// Include Inkplate library to the sketch
#include <Inkplate.h>

// Including fonts
#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSans9pt7b.h>

// Includes
#include <algorithm>
#include <ctime>

#include "Network.h"
#include "config.h"

// Variables to keep count of when to get new data, and when to just update time
RTC_DATA_ATTR unsigned int refreshes = 0;
const int refreshesToGet = 10;

// Initiate out Inkplate object
Inkplate display(INKPLATE_3BIT);

// Our networking functions, see Network.cpp for info
Network network;

// Variables for time and raw event info
char date[64];
char *data;

// Struct for storing calender event info
struct entry
{
    char name[128];
    char time[128];
    char location[128];
    int day = -1;
    int timeStamp;
};

// Here we store calendar entries
int entriesNum = 0;
entry entries[128];

// All our functions declared below setup and loop
void drawInfo();
void drawTime();
void drawGrid();
void getToFrom(char *dst, char *from, char *to, int *day, int *timeStamp);
bool drawEvent(entry *event, int day, int beginY, int max_y, int *y_next);
int cmp(const void *a, const void *b);
void drawData();

void setup()
{
    Serial.begin(115200);

    data = (char *)ps_malloc(2000000LL);

    // Initial display settings
    display.begin();

    display.setRotation(ROTATION);
    display.setTextWrap(false);
    display.setTextColor(0, 7);

    if (refreshes == 0)
    {
        // Welcome screen
        display.setCursor(0, 0);
        display.println(F("Top!"));
        display.setCursor(5, 230);
        display.setTextSize(2);
        display.println(F("Welcome to Inkplate 10 Google Calendar example!"));
        display.setCursor(5, 250);
        display.println(F("Connecting to WiFi..."));
        display.display();
    }

    delay(5000);
    network.begin();

    // Keep trying to get data if it fails the first time
    while (!network.getData(data))
    {
        Serial.println("Failed getting data, retrying");
        delay(1000);
    }

    // Initial screen clearing
    display.clearDisplay();

    // Drawing all data, functions for that are above
    drawInfo();
    drawGrid();
    drawData();
    drawTime();

    // Can't do partial due to deepsleep
    display.display();

    // Increment refreshes
    ++refreshes;

    // Go to sleep before checking again
    esp_sleep_enable_timer_wakeup(1000ll * DELAY_MS);
    (void)esp_deep_sleep_start();
}

void loop()
{
    // Never here
}

// Function for drawing calendar info
void drawInfo()
{
    // Setting font and color
    display.setTextColor(0, 7);
    display.setFont(&FreeSans12pt7b);
    display.setTextSize(1);

    display.setCursor(20, 20);

    // Find email in raw data
    char temp[64];
    char *start = strstr(data, "X-WR-CALNAME:");

    // If not found return
    if (!start)
        return;

    // Find where it ends
    start += 13;
    char *end = strchr(start, '\n');

    strncpy(temp, start, end - start - 1);
    temp[end - start - 1] = 0;

    // Print it
    display.println(temp);
}

// Drawing what time it is
void drawTime()
{
    // Initial text settings
    display.setTextColor(0, 7);
    display.setFont(&FreeSans12pt7b);
    display.setTextSize(1);

    display.setCursor(500, 20);

    // Our function to get time
    network.getTime(date, &timezone, 0);

    int t = date[16];
    date[16] = 0;
    display.println(date);
    date[16] = t;
}

// Draw lines in which to put events
void drawGrid()
{
    // upper left and low right coordinates
    int x1 = OUTSIDE_BORDER_WIDTH, y1 = OUTSIDE_BORDER_TOP;
    int x2 = x1 + COLUMN_WIDTH*COLUMNS, y2 = SCREEN_HEIGHT - OUTSIDE_BORDER_BOTTOM;

    // Columns and rows
    int n = 1, m = COLUMNS;

    // Line drawing
    display.drawThickLine(x1, y1 + HEADER_HEIGHT, x2, y1 + HEADER_HEIGHT, 0, 2.0);
    for (int i = 0; i < n + 1; ++i)
    {
        display.drawThickLine(x1, (int)((float)y1 + (float)i * (float)(y2 - y1) / (float)n), x2,
                              (int)((float)y1 + (float)i * (float)(y2 - y1) / (float)n), 0, 2.0);
    }

    for (int i = 0; i < m + 1; ++i)
    {
        display.drawThickLine(
            x1 + i * COLUMN_WIDTH, y1,
            x1 + i * COLUMN_WIDTH, y2,
            0, 2.0
        );
    }

    for (int i = 0; i < m; ++i)
    {
        display.setFont(&FreeSans9pt7b);

        // Display day info using time offset
        char temp[64];
        network.getTime(temp, &timezone, i * 3600L * 24);
        temp[10] = 0;

        // calculate where to put text and print it
        display.setCursor(x1 + i * COLUMN_WIDTH + INSIDE_BORDER_WIDTH, y1 + HEADER_HEIGHT - 6);
        display.println(temp);
    }
}

// Format event times, example 13:00 to 14:00
void getToFrom(char *dst, char *from, char *to, int *day, int *timeStamp)
{
    // ANSI C time struct
    struct tm ltm = {0}, ltm2 = {0};
    char temp[128], temp2[128];
    strncpy(temp, from, 16);
    temp[16] = 0;

    // https://github.com/esp8266/Arduino/issues/5141, quickfix
    memmove(temp + 5, temp + 4, 16);
    memmove(temp + 8, temp + 7, 16);
    memmove(temp + 14, temp + 13, 16);
    memmove(temp + 16, temp + 15, 16);
    temp[4] = temp[7] = temp[13] = temp[16] = '-';

    // time.h function
    strptime(temp, "%Y-%m-%dT%H-%M-%SZ", &ltm);

    // create start and end event structs
    struct tm event, event2;
    time_t epoch = timezone.toLocal(mktime(&ltm));
    gmtime_r(&epoch, &event);
    strncpy(dst, asctime(&event) + 11, 5);

    dst[5] = '-';

    strncpy(temp2, to, 16);
    temp2[16] = 0;

    // Same as above

    // https://github.com/esp8266/Arduino/issues/5141, quickfix
    memmove(temp2 + 5, temp2 + 4, 16);
    memmove(temp2 + 8, temp2 + 7, 16);
    memmove(temp2 + 14, temp2 + 13, 16);
    memmove(temp2 + 16, temp2 + 15, 16);
    temp2[4] = temp2[7] = temp2[13] = temp2[16] = '-';

    strptime(temp2, "%Y-%m-%dT%H-%M-%SZ", &ltm2);

    time_t epoch2 = timezone.toLocal(mktime(&ltm2));
    gmtime_r(&epoch2, &event2);
    strncpy(dst + 6, asctime(&event2) + 11, 5);

    dst[11] = 0;

    {
        time_t nowSecs = timezone.toLocal(time(nullptr));
        int nowDays = nowSecs / (24 * 60 * 60);
        int eventDays = epoch / (24 * 60 * 60);

        *day = eventDays - nowDays;
        if (*day < 0 || *day >= COLUMNS) {
            *day = -1;
        }
    }
}

// Function to draw event
bool drawEvent(entry *event, int day, int beginY, int max_y, int *y_next)
{
    // Upper left coordintes
    int x1 = OUTSIDE_BORDER_WIDTH + INSIDE_BORDER_WIDTH + COLUMN_WIDTH * day;
    int y1 = beginY + INSIDE_SPACING_HEIGHT;
    int max_width_text = COLUMN_WIDTH - 2*INSIDE_BORDER_WIDTH;
    display.setCursor(x1 + INSIDE_BORDER_WIDTH, y1 + 30);

    // Setting text font
    display.setFont(&FreeSans12pt7b);

    // Some temporary variables
    int n = 0;
    char line[128];

    // Insert line brakes into setTextColor
    int lastSpace = -100;
    for (int i = 0; i < min((size_t)64, strlen(event->name)); ++i)
    {
        // Copy name letter by letter and check if it overflows space given
        line[n] = event->name[i];
        if (line[n] == ' ')
            lastSpace = n;
        line[++n] = 0;

        int16_t xt1, yt1;
        uint16_t w, h;

        // Gets text bounds
        display.getTextBounds(line, 0, 0, &xt1, &yt1, &w, &h);

        // Char out of bounds, put in next line
        if (w > max_width_text)
        {
            // if there was a space 5 chars before, break line there
            if (n - lastSpace < 5)
            {
                i -= n - lastSpace - 1;
                line[lastSpace] = 0;
            }

            // Print text line
            display.setCursor(x1 + INSIDE_BORDER_WIDTH, display.getCursorY());
            display.println(line);

            // Clears line (null termination on first charachter)
            line[0] = 0;
            n = 0;
        }
    }

    // display last line
    display.setCursor(x1 + 5, display.getCursorY());
    display.println(line);

    // Set cursor on same y but change x
    display.setCursor(x1 + 3, display.getCursorY());
    display.setFont(&FreeSans9pt7b);

    // Print time
    // also, if theres a location print it
    if (strlen(event->location) != 1)
    {
        display.println(event->time);

        display.setCursor(x1 + 5, display.getCursorY());

        char line[128] = {0};

        for (int i = 0; i < strlen(event->location); ++i)
        {
            line[i] = event->location[i];
            line[i + 1] = 0;

            int16_t xt1, yt1;
            uint16_t w, h;

            // Gets text bounds
            display.getTextBounds(line, 0, 0, &xt1, &yt1, &w, &h);

            if (w > max_width_text)
            {
                for (int j = i - 1; j > max(-1, i - 4); --j)
                    line[j] = '.';
                line[i] = 0;
            }
        }

        display.print(line);
    }
    else
    {
        display.print(event->time);
    }

    int bx1 = x1 + 1;
    int by1 = y1;
    int bx2 = x1 + COLUMN_WIDTH - INSIDE_BORDER_WIDTH - INSIDE_BORDER_WIDTH - 2;
    int by2 = display.getCursorY() + 7;

    // Draw event rect bounds
    display.drawThickLine(bx1, by1, bx1, by2, 0, 2.0);
    display.drawThickLine(bx1, by2, bx2, by2, 0, 2.0);
    display.drawThickLine(bx2, by2, bx2, by1, 0, 2.0);
    display.drawThickLine(bx2, by1, bx1, by1, 0, 2.0);

    // Set how high is the event
    *y_next = by2 + 1;

    // Return is it overflowing
    return display.getCursorY() < max_y;
}

// Struct event comparison function, by timestamp, used for qsort later on
int cmp(const void *a, const void *b)
{
    entry *entryA = (entry *)a;
    entry *entryB = (entry *)b;

    return (entryA->timeStamp - entryB->timeStamp);
}

// Main data drawing data
void drawData()
{
    long i = 0;
    long n = strlen(data);

    // reset count
    entriesNum = 0;

    // Search raw data for events
    while (i < n && strstr(data + i, "BEGIN:VEVENT"))
    {
        // Find next event start and end
        i = strstr(data + i, "BEGIN:VEVENT") - data + 12;
        char *end = strstr(data + i, "END:VEVENT");

        if (end == NULL)
            continue;

        // Find all relevant event data
        char *summary = strstr(data + i, "SUMMARY:") + 8;
        char *location = strstr(data + i, "LOCATION:") + 9;
        char *timeStart = strstr(data + i, "DTSTART:") + 8;
        char *timeEnd = strstr(data + i, "DTEND:") + 6;

        if (summary && summary < end)
        {
            strncpy(entries[entriesNum].name, summary, strchr(summary, '\n') - summary);
            entries[entriesNum].name[strchr(summary, '\n') - summary] = 0;
        }
        if (location && location < end)
        {
            strncpy(entries[entriesNum].location, location, strchr(location, '\n') - location);
            entries[entriesNum].location[strchr(location, '\n') - location] = 0;
        }
        if (timeStart && timeStart < end && timeEnd < end)
        {
            getToFrom(entries[entriesNum].time, timeStart, timeEnd, &entries[entriesNum].day,
                      &entries[entriesNum].timeStamp);
        }
        ++entriesNum;
    }

    // Sort entries by time
    qsort(entries, entriesNum, sizeof(entry), cmp);

    // Events displayed and overflown counters
    int columns[COLUMNS] = {0};
    bool clogged[COLUMNS] = {0};
    int cloggedCount[COLUMNS] = {0};

    for (int i = 0; i < COLUMNS; ++i) {
        columns[i] = OUTSIDE_BORDER_TOP + HEADER_HEIGHT + 1;
    }

    // Displaying events one by one
    for (int i = 0; i < entriesNum; ++i)
    {
        // If column overflowed just add event to not shown
        if (entries[i].day != -1 && clogged[entries[i].day])
            ++cloggedCount[entries[i].day];
        if (entries[i].day == -1 || clogged[entries[i].day])
            continue;

        // We store how much height did one event take up
        int shift = 0;
        bool s = drawEvent(&entries[i], entries[i].day, columns[entries[i].day], SCREEN_HEIGHT - OUTSIDE_BORDER_BOTTOM, &shift);
        columns[entries[i].day] = shift;

        // If it overflowed, set column to clogged and add one event as not shown
        if (!s)
        {
            ++cloggedCount[entries[i].day];
            clogged[entries[i].day] = 1;
        }
    }

    // Display not shown events info
    for (int i = 0; i < COLUMNS; ++i)
    {
        if (clogged[i])
        {
            // Draw notification showing that there are more events than drawn ones
            display.fillRoundRect(OUTSIDE_BORDER_WIDTH + i * COLUMN_WIDTH + INSIDE_BORDER_WIDTH, SCREEN_HEIGHT - OUTSIDE_BORDER_BOTTOM - INSIDE_BORDER_WIDTH - 24, COLUMN_WIDTH - 2*INSIDE_BORDER_WIDTH, 20, 10, 0);
            display.setCursor(OUTSIDE_BORDER_WIDTH + i * COLUMN_WIDTH + INSIDE_BORDER_WIDTH + 10, SCREEN_HEIGHT - OUTSIDE_BORDER_BOTTOM - INSIDE_BORDER_WIDTH - 24 + 15);
            display.setTextColor(7, 0);
            display.setFont(&FreeSans9pt7b);
            display.print(cloggedCount[i]);
            display.print(" more events");
        }
    }
}
