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
#include <uICAL.h>
#include <uICAL/datecalc.h>
#include <sstream>

#include "Network.h"
#include "config.h"

// Variables to keep count of when to get new data, and when to just update time
RTC_DATA_ATTR unsigned int refreshes = 0;

// Initiate out Inkplate object
Inkplate display(INKPLATE_3BIT);

// Our networking functions, see Network.cpp for info
Network network;

// Struct for storing calender event info
struct entry
{
    String summary;
    String location;
    uICAL::DateTime start;
    uICAL::DateTime end;
    int day;
};

// All our functions declared below setup and loop
void drawInfo();
void drawTime();
void drawGrid();
bool drawEvent(const entry &event, int day, int beginY, int max_y, int *y_next);
void drawData(String &data);

void setup()
{
    Serial.begin(115200);
    Serial.println("Starting");

    // Initial display settings
    display.begin();

    display.setRotation(ROTATION);
    display.setTextWrap(false);
    display.setTextColor(0, 7);

    if (refreshes == 0)
    {
        // Welcome screen
        display.setCursor(5, 230);
        display.setTextSize(2);
        display.println(F("Welcome to Inkplate 10 Google Calendar!"));
        display.setCursor(5, 250);
        display.println(F("Connecting to WiFi..."));
        display.display();
    }

    delay(5000);
    network.begin();

    // Keep trying to get data if it fails the first time
    String data;
    while (!network.getData(&data))
    {
        Serial.println("Failed getting data, retrying");
        delay(1000);
    }

    // Initial screen clearing
    display.println(F("Connected"));
    display.clearDisplay();

    // Drawing all data, functions for that are above
    drawInfo();
    drawGrid();
    drawData(data);
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

    // Print it
    display.println("Common Calendar");
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
    time_t now_secs = timezone.toLocal(time(nullptr));
    uICAL::DateTime now = now_secs;

    display.println(now.format("%c").c_str());
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
        time_t day_time_secs = timezone.toLocal(time(nullptr)) + i * 3600L * 24;
        uICAL::dhms_t day_time_dhms = uICAL::to_dhms(day_time_secs);
        day_time_secs = uICAL::to_seconds(std::get<0>(day_time_dhms), 0, 0, 0);
        uICAL::DateTime day_time = day_time_secs;

        // calculate where to put text and print it
        display.setCursor(x1 + i * COLUMN_WIDTH + INSIDE_BORDER_WIDTH, y1 + HEADER_HEIGHT - 6);
        display.println(day_time.format("%a %d/%h").c_str());
    }
}

// Function to draw event
bool drawEvent(const entry &event, int day, int beginY, int max_y, int *y_next)
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
    for (int i = 0; i < event.summary.length(); ++i)
    {
        // Copy name letter by letter and check if it overflows space given
        line[n] = event.summary[i];
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
    String time = event.start.format("%H:%M") + " to " + event.end.format("%H:%M");

    // also, if theres a location print it
    if (event.location.length() > 0)
    {
        display.println(time.c_str());

        display.setCursor(x1 + 5, display.getCursorY());

        char line[128] = {0};

        for (int i = 0; i < event.location.length(); ++i)
        {
            line[i] = event.location[i];
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
        display.print(time.c_str());
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

// Struct event comparison function, by timestamp, used for sort later on
bool compare_entries(const entry &a, const entry &b)
{
    return a.start < b.start;
}

uICAL::DateTime utc_datetime_to_local(const uICAL::DateTime &dt) {
    uICAL::seconds_t secs = timezone.toLocal(dt.epochtime.epochSeconds);
    return uICAL::DateTime(secs);
}

// Main data drawing data
void drawData(String &data)
{
    // calculate begin and end times
    Serial.println("drawData() begin");
    uICAL::dhms_t begin_dhms = uICAL::to_dhms(timezone.toLocal(time(nullptr)));
    int begin_days = std::get<0>(begin_dhms);
    int end_days = begin_days + COLUMNS;

    uICAL::DateTime begin(uICAL::to_seconds(begin_days, 0, 0, 0));
    uICAL::DateTime end(uICAL::to_seconds(end_days, 0, 0, 0));

    // Here we store calendar entries
    std::vector<entry> entries;

    try {
        Serial.println("drawData() parsing entries");
        uICAL::string meow = uICAL::string(data);
        uICAL::istream_String istm(meow);
        uICAL::string line;
        auto cal = uICAL::Calendar::load(istm, [=](const uICAL::VEvent& event){
            return true;
        });
        auto calIt = uICAL::new_ptr<uICAL::CalendarIter>(cal, begin, end);
        Serial.println("drawData() done parsing entries");
        while(calIt->next()) {
            uICAL::CalendarEntry_ptr src_entry = calIt->current();

            // Find all relevant event data
            String summary = src_entry->summary();
            String location = src_entry->location();
            uICAL::DateTime start(src_entry->start());
            uICAL::DateTime end(src_entry->end());

            struct entry entry;
            entry.summary = summary;
            entry.location = location;
            entry.start = utc_datetime_to_local(start);
            entry.end = utc_datetime_to_local(end);

            uICAL::dhms_t start_dhms = entry.start.convert_to_dhms();
            entry.day = std::get<0>(start_dhms) - std::get<0>(begin_dhms);
            if (entry.day > 0 && entry.day < COLUMNS) {
                entries.push_back(entry);
            }
        }
    }
    catch (uICAL::Error ex) {
        Serial.printf("%s: %s\n", ex.message.c_str(), "! Failed loading calendar");
    }

    // Sort entries by time
    Serial.println("drawData() sorting");
    sort(entries.begin(), entries.end(), compare_entries);

    // Events displayed and overflown counters
    Serial.println("drawData() displaying");
    int columns[COLUMNS] = {0};
    int cloggedCount[COLUMNS] = {0};

    for (int i = 0; i < COLUMNS; ++i) {
        columns[i] = OUTSIDE_BORDER_TOP + HEADER_HEIGHT + 1;
    }

    // Displaying events one by one
    for (auto entry : entries) {
        // If column overflowed just add event to not shown
        if (cloggedCount[entry.day] > 0) {
            ++cloggedCount[entry.day];
            continue;
        }

        // We store how much height did one event take up
        int y_pos = 0;
        bool s = drawEvent(entry, entry.day, columns[entry.day], SCREEN_HEIGHT - OUTSIDE_BORDER_BOTTOM, &y_pos);
        columns[entry.day] = y_pos;

        // If it overflowed, set column to clogged and add one event as not shown
        if (!s)
        {
            ++cloggedCount[entry.day];
        }
    }

    // Display not shown events info
    for (int i = 0; i < COLUMNS; ++i)
    {
        if (cloggedCount[i])
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
