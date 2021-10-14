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
#include <memory>
#include <uICAL.h>
#include <uICAL/date.h>
#include <uICAL/time.h>
#include <uICAL/datecalc.h>
#include <sstream>

#include "Network.h"
#include "config.h"
#include "local_time.h"

Timezone_ptr timezone_ptr = std::make_shared<Timezone>(timezone);
Local_TZ_ptr local_tz = std::make_shared<Local_TZ>(Local_TZ(timezone_ptr));

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
    bool start_has_time;
    bool end_has_time;
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
    Serial.println("Starting.");

    // Initial display settings
    display.begin();

    Serial.println("Configuring display.");
    display.setRotation(ROTATION);
    display.setTextWrap(false);
    display.setTextColor(0, 7);

    if (refreshes == 0)
    {
        // Welcome screen
        Serial.println("Drawing welcome screen.");
        display.setCursor(5, 230);
        display.setTextSize(2);
        display.println(F("Welcome to Inkplate 10 Google Calendar!"));
        display.setCursor(5, 250);
        display.println(F("Connecting to WiFi..."));
        display.display();
    }

    Serial.println("Going to sleep.");
    // delay(5000);

    Serial.println("Starting network stuff.");
    network.begin();

    // Keep trying to get data if it fails the first time
    String data;
    Serial.println("Getting data.");
    while (!network.getData(&data))
    {
        Serial.println("Failed getting data, retrying");
        delay(1000);
    }

    // Initial screen clearing
    Serial.println("Got data.");
    display.clearDisplay();

    // Drawing all data, functions for that are above
    Serial.println("Drawing.");
    drawInfo();
    drawGrid();
    drawData(data);
    drawTime();

    // Can't do partial due to deepsleep
    Serial.println("Updating display.");
    display.display();

    // Increment refreshes
    ++refreshes;

    // Go to sleep before checking again
    Serial.println("Going to sleep. Goodnight.");
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
    uICAL::DateTime utc_datetime(time(nullptr), uICAL::tz_UTC);
    uICAL::DateTime local_datetime = utc_datetime.shift_timezone(local_tz);
    display.println(local_datetime.format("%c").c_str());
}

void draw_error(const String &msg) {
    const char *buffer = msg.c_str();

    int16_t xt1, yt1;
    uint16_t w, h;

    // Gets text bounds
    display.getTextBounds(buffer, 0, 0, &xt1, &yt1, &w, &h);

    const uint16_t border = 10;
    const uint16_t x_text = SCREEN_WIDTH/2 - w/2;
    const uint16_t y_text = SCREEN_HEIGHT/2 + h/2;

    const uint16_t x_left = SCREEN_WIDTH/2 - w/2 - border;
    const uint16_t y_top = SCREEN_HEIGHT/2 - h/2 - border;
    const uint16_t width = w + border * 2;
    const uint16_t height = h + border * 2;

    display.drawRoundRect(x_left, y_top, width, height, 0, 0);
    display.setCursor(x_text, y_text);
    display.print(buffer);
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

    uICAL::DateTime utc_datetime(time(nullptr), uICAL::tz_UTC);
    uICAL::DateTime local_datetime = utc_datetime.shift_timezone(local_tz);
    uICAL::Date local_date = local_datetime.date();

    for (int i = 0; i < m; ++i)
    {
        // Calculate date for column
        uICAL::Date date = local_date + i;

        // calculate where to put text and print it
        display.setFont(&FreeSans9pt7b);
        display.setCursor(x1 + i * COLUMN_WIDTH + INSIDE_BORDER_WIDTH, y1 + HEADER_HEIGHT - 6);
        display.println(date.format("%a %d/%h").c_str());
    }
}

// Function to draw event
bool drawEvent(const entry &event, int day, int beginY, int max_y, int *y_next)
{
    // Upper left coordintes
    int x1 = OUTSIDE_BORDER_WIDTH + INSIDE_BORDER_WIDTH + COLUMN_WIDTH * day;
    int y1 = beginY + INSIDE_SPACING_HEIGHT;

    int x_text = x1 + INSIDE_BORDER_WIDTH;
    int max_width_text = COLUMN_WIDTH - 4*INSIDE_BORDER_WIDTH;
    display.setCursor(x_text, y1 + 30);

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
        // Serial.printf("'%s' %d %d %d %d %d\n", line, max_width_text, w, w > max_width_text, lastSpace, n - lastSpace);

        // Char out of bounds, put in next line
        // Note n is now pointing at terminating 0 in line
        if (w > max_width_text)
        {
            if (n - lastSpace - 1 < 5)
            {
                // if there was a space 5 chars before, break line there
                i -= n - lastSpace - 1;
                line[lastSpace] = 0;
            } else {
                // otherwise, break before current character
                i -= 1;
                line[n-1] = 0;
            }

            // Print text line
            display.setCursor(x_text, display.getCursorY());
            display.println(line);

            // Clears line (null termination on first charachter)
            line[0] = 0;
            n = 0;
        }
    }

    // display last line
    display.setCursor(x_text, display.getCursorY());
    display.println(line);

    // Set cursor on same y but change x
    display.setCursor(x_text, display.getCursorY());
    display.setFont(&FreeSans9pt7b);

    // Print time
    String time;
    unsigned days = uICAL::Date(event.end) - uICAL::Date(event.start);
    if (event.start_has_time && event.end_has_time) {
        time = event.start.format("%H:%M") + " to " + event.end.format("%H:%M");
        if (days > 0) {
            time = time + "+" + String(days) + " days";
        }
    } else {
        time = String(days) + " days";
        if (days == 1) {
            time = "All day";
        } else {
            time = String(days) + " days";
        }
    }

    // also, if theres a location print it
    if (event.location.length() > 0)
    {
        display.println(time);

        display.setCursor(x_text, display.getCursorY());

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
    return dt.shift_timezone(local_tz);
}

// Main data drawing data
void drawData(String &data)
{
    // calculate begin and end times
    Serial.println("drawData() begin");


    uICAL::DateTime utc_datetime(time(nullptr), uICAL::tz_UTC);
    uICAL::DateTime local_datetime = utc_datetime.shift_timezone(local_tz);
    uICAL::Date begin_date = local_datetime.date();
    uICAL::Date end_date = begin_date + COLUMNS;
    uICAL::DateTime begin = begin_date.start_of_day(local_tz).shift_timezone(uICAL::tz_UTC);
    uICAL::DateTime end = end_date.start_of_day(local_tz).shift_timezone(uICAL::tz_UTC);

    Serial.println("utc_datetime: "+ utc_datetime.as_str());
    Serial.println("local_datetime: "+ local_datetime.as_str());
    Serial.println("begin_date/end_date: "+ begin_date.as_str() + " / " + end_date.as_str());
    Serial.println("begin/end: "+ begin.as_str() + " / " + end.as_str());

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

            // Find all relevant event data.
            String summary = src_entry->summary();
            String location = src_entry->location();
            uICAL::DateTime start(src_entry->start());
            uICAL::DateTime end(src_entry->end());
            uICAL::Date start_date = start.date();
            uICAL::Date end_date = end.date();

            // Fill in our struct with data.
            struct entry entry;
            entry.summary = summary;
            entry.location = location;
            entry.start = start.shift_timezone(local_tz);
            entry.end = end.shift_timezone(local_tz);
            entry.start_has_time = src_entry->start_has_time;
            entry.end_has_time = src_entry->end_has_time;
            entry.day = start_date - begin_date;

            // For all day events set to local start of day.
            if (!src_entry->start_has_time) {
                entry.start = start_date.start_of_day(local_tz);
            }
            if (!src_entry->end_has_time) {
                entry.end = end_date.start_of_day(local_tz);
            }

            // If entry withing date bounds, add to list.
            // Entry should always be within bounds here, but check just in case.
            if (entry.day >= 0 && entry.day < COLUMNS) {
                Serial.println("----------");
                Serial.println("DAY " + entry.day);
                Serial.println(src_entry->as_str());
                entries.push_back(entry);
            } else {
                Serial.println("++++++++++");
                Serial.println("DAY " + entry.day);
                Serial.println(src_entry->as_str());
            }
        }
    }
    catch (uICAL::Error ex) {
        Serial.printf("%s: %s\n", ex.message.c_str(), "! Failed loading calendar");
        draw_error("Failed loading calendar.");
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
