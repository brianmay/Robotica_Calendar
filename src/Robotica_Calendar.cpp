// Include Inkplate library to the sketch
#include <Inkplate.h>

// Including fonts
#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSans9pt7b.h>

// Includes
#include <algorithm>
#include <ctime>
#include <memory>
#include <sstream>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#include "Network.h"
#include "local_time.h"
#include "config.h"
#include "mystring.h"
#include "tz.h"
#include "epochtime.h"
#include "date.h"
#include "datetime.h"
#include "mytime.h"

namespace Project
{
  using Timezone_ptr = std::shared_ptr<Timezone>;
  using Local_TZ_ptr = std::shared_ptr<Local_TZ>;

  Timezone_ptr timezone_ptr = std::make_shared<Timezone>(timezone);
  Local_TZ_ptr local_tz = std::make_shared<Local_TZ>(Local_TZ(timezone_ptr));

  WiFiClient espClient;
  PubSubClient client(espClient);

  // Variables to keep count of when to get new data, and when to just update time
  RTC_DATA_ATTR bool refreshed = false;

  // Initiate out Inkplate object
  Inkplate display(INKPLATE_3BIT);

  // Our networking functions, see Network.cpp for info
  Network network;

  enum status_t
  {
    pending,
    in_progress,
    completed,
    cancelled
  };
  // Struct for storing calender event info
  struct entry
  {
    String title;
    DateTime start_time;
    DateTime end_time;
    Date start_date;
    Date end_date;
    int day;
    status_t status;

    entry(String &summary, DateTime &start_time, DateTime &end_time, Date &start_date, Date &end_date, int day, status_t status);
  };

  // Constructor for entry
  entry::entry(String &summary, DateTime &start_time, DateTime &end_time, Date &start_date, Date &end_date, int day, status_t status) : title(summary), start_time(start_time), end_time(end_time), start_date(start_date), end_date(end_date), day(day), status(status) {}

  // All our functions declared below setup and loop
  void drawInfo();
  void drawTime();
  void drawGrid();
  bool drawEvent(const entry &event, int day, int beginY, int max_y, int *y_next);
  void drawData(const JsonArray &array);
  void callback(char *topic, byte *message, unsigned int length);

  void reconnect()
  {
    // Loop until we're reconnected
    while (!client.connected())
    {
      Serial.print("Attempting MQTT connection...");
      // Attempt to connect
      if (client.connect("ESP32Client", mqtt_username, mqtt_password))
      {
        Serial.println("connected");
        // Subscribe
        client.subscribe(mqtt_topic);
      }
      else
      {
        Serial.print("failed, rc=");
        Serial.print(client.state());
        Serial.println(" try again in 5 seconds");
        // Wait 5 seconds before retrying
        delay(5000);
      }
    }
  }

  void callback(char *topic, byte *message, unsigned int length)
  {
    DynamicJsonDocument doc(30 * 1024);
    DeserializationError error = deserializeJson(doc, message);

    // Test if parsing succeeds.
    if (error)
    {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      return;
    }

    // Drawing all data, functions for that are above
    display.clearDisplay();
    drawInfo();
    drawGrid();
    drawTime();
    const JsonArray array = doc.as<JsonArray>();
    drawData(array);
    display.display();
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
    DateTime now = DateTime::local_now(local_tz);
    display.println(now.format("%c").c_str());
  }

  void draw_error(const String &msg)
  {
    const char *buffer = msg.c_str();

    int16_t xt1, yt1;
    uint16_t w, h;

    // Gets text bounds
    display.getTextBounds(buffer, 0, 0, &xt1, &yt1, &w, &h);

    const uint16_t border = 10;
    const uint16_t x_text = SCREEN_WIDTH / 2 - w / 2;
    const uint16_t y_text = SCREEN_HEIGHT / 2 + h / 2;

    const uint16_t x_left = SCREEN_WIDTH / 2 - w / 2 - border;
    const uint16_t y_top = SCREEN_HEIGHT / 2 - h / 2 - border;
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
    int x2 = x1 + COLUMN_WIDTH * COLUMNS, y2 = SCREEN_HEIGHT - OUTSIDE_BORDER_BOTTOM;

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
          0, 2.0);
    }

    DateTime local_datetime = DateTime::local_now(local_tz);
    Date local_date = local_datetime.date();

    for (int i = 0; i < m; ++i)
    {
      // Calculate date for column
      Date date = local_date + i;

      // calculate where to put text and print it
      display.setFont(&FreeSans9pt7b);
      display.setCursor(x1 + i * COLUMN_WIDTH + INSIDE_SPACING_WIDTH, y1 + HEADER_HEIGHT - 6);
      display.println(date.format("%a %d/%h").c_str());
    }
  }

  // Function to draw event
  bool drawEvent(const Date &local_date, const entry &event, int day, int beginY, int max_y, int *y_next)
  {
    // Upper left coordinates
    int x1 = OUTSIDE_BORDER_WIDTH + INSIDE_SPACING_WIDTH + COLUMN_WIDTH * day;
    int y1 = beginY + INSIDE_SPACING_HEIGHT;

    int x_text = x1 + EVENT_SPACING_WIDTH;
    int max_width_text = COLUMN_WIDTH - 2 * INSIDE_SPACING_WIDTH - 2 * EVENT_SPACING_WIDTH;
    display.setCursor(x_text, y1 + 20 + EVENT_SPACING_HEIGHT);

    // Setting text font
    display.setFont(&FreeSans12pt7b);

    {
      // Some temporary variables
      int n = 0;
      char line[128];

      // Insert line brakes into setTextColor
      int lastSpace = -100;
      for (int i = 0; i < event.title.length(); ++i)
      {
        // Copy name letter by letter and check if it overflows space given
        line[n] = event.title[i];
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
          if (n - lastSpace - 1 < 15)
          {
            // if there was a space 15 chars before, break line there
            i -= n - lastSpace - 1;
            line[lastSpace] = 0;
          }
          else
          {
            // otherwise, break before current character
            i -= 1;
            line[n - 1] = 0;
          }

          // Print text line
          display.setCursor(x_text, display.getCursorY());
          display.println(line);

          // Clears line (null termination on first character)
          line[0] = 0;
          n = 0;
        }
      }

      // display last line
      display.setCursor(x_text, display.getCursorY());
      display.println(line);
    }

    // Set cursor on same y but change x
    display.setCursor(x_text, display.getCursorY());
    display.setFont(&FreeSans9pt7b);

    // Print time
    String time;
    unsigned start_days = local_date - event.start_date;
    time = event.start_time.format("%H:%M");

    time = time + " to ";

    unsigned end_days = event.end_date - local_date;
    if (end_days > 0)
    {
      time = time + event.end_time.format("%H:%M") + "+" + String(end_days) + " days";
    }
    else
    {
      time = time + event.end_time.format("%H:%M");
    }

    display.print(time);

    int bx1 = x1 + 1;
    int by1 = y1;
    int bx2 = x1 + COLUMN_WIDTH - INSIDE_SPACING_WIDTH - INSIDE_SPACING_WIDTH - 2;
    int by2 = display.getCursorY() + EVENT_SPACING_HEIGHT;

    float width = 0;
    switch (event.status)
    {
    case pending:
      width = 1;
      break;
    case in_progress:
      width = 2;
      break;
    case completed:
      width = 0;
      break;
    case cancelled:
      width = 0;
      break;
    }

    // Draw event rect bounds
    for (int border = 0; border < width; border = border + 1)
    {
      int cx1 = bx1 + border * 4;
      int cy1 = by1 + border * 4;
      int cx2 = bx2 - border * 4;
      int cy2 = by2 - border * 4;
      display.drawThickLine(cx1, cy1, cx1, cy2, 0, 1);
      display.drawThickLine(cx1, cy2, cx2, cy2, 0, 1);
      display.drawThickLine(cx2, cy2, cx2, cy1, 0, 1);
      display.drawThickLine(cx2, cy1, cx1, cy1, 0, 1);
    }

    // Set how high is the event
    *y_next = by2 + 1;

    // Return is it overflowing
    return display.getCursorY() < max_y;
  }

  // Struct event comparison function, by timestamp, used for sort later on
  bool compare_entries(const entry &a, const entry &b)
  {
    return a.start_time < b.start_time;
  }

  void convertFromJson(JsonVariantConst src, DateTime &dst)
  {
    const char *required_time = src.as<const char *>();

    tm timeinfo;
    strptime(required_time, "%FT%TZ", &timeinfo);

    time_t time = mktime(&timeinfo);
    dst = DateTime(time, tz_UTC);
  }

  void convertFromJson(JsonVariantConst src, DatePeriod &dst)
  {
    string str = string(src.as<const char *>());
    size_t pos;

    pos = str.find(":");
    if (pos == std::string::npos)
    {
      return;
    }
    int hours = str.substr(0, pos).as_int();
    str = str.substr(pos + 1);

    pos = str.find(":");
    if (pos == std::string::npos)
    {
      return;
    }
    int minutes = str.substr(0, pos).as_int();
    str = str.substr(pos + 1);
    int seconds = str.as_int();

    dst = DatePeriod(0, hours, minutes, seconds);
  }

  // Main data drawing data
  void drawData(const JsonArray &array)
  {
    // calculate begin and end times
    Serial.println("drawData() begin");

    DateTime utc_datetime = DateTime::utc_now();
    DateTime local_datetime = utc_datetime.shift_timezone(local_tz);
    Date begin_date = local_datetime.date();
    Date end_date = begin_date + COLUMNS;
    DateTime begin = begin_date.start_of_day(local_tz).shift_timezone(tz_UTC);
    DateTime end = end_date.start_of_day(local_tz).shift_timezone(tz_UTC);

    Serial.println("utc_datetime: " + utc_datetime.as_str());
    Serial.println("local_datetime: " + local_datetime.as_str());
    Serial.println("begin_date/end_date: " + begin_date.as_str() + " / " + end_date.as_str());
    Serial.println("begin/end: " + begin.as_str() + " / " + end.as_str());

    // Here we store calendar entries
    std::vector<entry> entries;

    Serial.println("drawData() parsing entries");

    for (JsonVariant src_entry : array)
    {
      // Find all relevant event data.
      String summary = src_entry["title"];
      String importance = src_entry["importance"];
      String status_str = src_entry["status"];
      DateTime entry_start_time = src_entry["start_time"];
      // DatePeriod duration = src_entry["required_duration"];
      DateTime entry_end_time = src_entry["end_time"];

      entry_start_time = entry_start_time.shift_timezone(local_tz);
      entry_end_time = entry_end_time.shift_timezone(local_tz);

      Date entry_start_date = entry_start_time.date();
      Date entry_end_date = entry_end_time.date();

      int day = entry_start_date - begin_date;
      status_t status;

      if (status_str == "Completed")
      {
        status = completed;
      }
      else if (status_str == "Cancelled")
      {
        status = cancelled;
      }
      else if (status_str == "InProgress")
      {
        status = in_progress;
      }
      else
      {
        status = pending;
      }

      // If entry already started but not finished, then set to day 0.
      if (day < 0 && entry_end_date >= begin_date)
      {
        day = 0;
      }

      // Fill in our struct with data.
      struct entry entry(summary, entry_start_time, entry_end_time, entry_start_date, entry_end_date, day, status);

      // If entry within date bounds, add to list.
      if (entry.day >= 0 && entry.day < COLUMNS)
      {
        Serial.println("----------");
        entries.push_back(entry);
      }
      else
      {
        Serial.println("++++++++++");
      }

      Serial.println("DAY " + String(entry.day));
      Serial.println("summary " + entry.title);
      Serial.println("status " + status_str + " " + String(entry.status));
      Serial.println("start " + entry.start_time.as_str());
      Serial.println("start " + entry.start_date.as_str());
      Serial.println("end " + entry.end_time.as_str());
      Serial.println("end " + entry.end_date.as_str());
      Serial.println();
    }

    // Sort entries by time
    // Serial.println("drawData() sorting");
    // sort(entries.begin(), entries.end(), compare_entries);

    // Events displayed and overflown counters
    int columns[COLUMNS] = {0};
    int cloggedCount[COLUMNS] = {0};

    for (int i = 0; i < COLUMNS; ++i)
    {
      columns[i] = OUTSIDE_BORDER_TOP + HEADER_HEIGHT + 1;
    }

    // Displaying events one by one
    for (auto entry : entries)
    {
      // If column overflowed just add event to not shown
      if (cloggedCount[entry.day] > 0)
      {
        ++cloggedCount[entry.day];
        continue;
      }

      // We store how much height did one event take up
      int y_pos = 0;
      Date local_date = local_datetime.date() + entry.day;
      bool s = drawEvent(local_date, entry, entry.day, columns[entry.day], SCREEN_HEIGHT - OUTSIDE_BORDER_BOTTOM, &y_pos);
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
        display.fillRoundRect(OUTSIDE_BORDER_WIDTH + i * COLUMN_WIDTH + INSIDE_SPACING_WIDTH, SCREEN_HEIGHT - OUTSIDE_BORDER_BOTTOM - INSIDE_SPACING_WIDTH - 24, COLUMN_WIDTH - 2 * INSIDE_SPACING_WIDTH, 20, 10, 0);
        display.setCursor(OUTSIDE_BORDER_WIDTH + i * COLUMN_WIDTH + INSIDE_SPACING_WIDTH + 10, SCREEN_HEIGHT - OUTSIDE_BORDER_BOTTOM - INSIDE_SPACING_WIDTH - 24 + 15);
        display.setTextColor(7, 0);
        display.setFont(&FreeSans9pt7b);
        display.print(cloggedCount[i]);
        display.print(" more events");
      }
    }
  }
}

using namespace Project;

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

  if (!refreshed)
  {
    // Welcome screen
    Serial.println("Drawing welcome screen.");
    display.setCursor(5, 230);
    display.setTextSize(2);
    display.println(F("Welcome to Robotica Calendar!"));
    display.setCursor(5, 250);
    display.println(F("Connecting to WiFi..."));
    display.display();
  }

  Serial.println("Going to sleep.");
  // delay(5000);

  Serial.println("Starting network stuff.");
  network.begin();

  // Keep trying to get data if it fails the first time
  Serial.println("Waiting for connection.");
  network.waitForConnection();

  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  client.setKeepAlive(5 * 60);
  client.setBufferSize(30 * 1024);

  // Initial screen clearing
  Serial.println("Got connection.");

  // Set refreshed
  refreshed = true;

  // Go to sleep before checking again
  Serial.println("Going to sleep. Goodnight.");
  // esp_sleep_enable_timer_wakeup(1000ll * DELAY_MS);
  // (void)esp_deep_sleep_start();
}

void loop()
{
  if (!client.connected())
  {
    reconnect();
  }
  client.loop();
}
