#ifndef SPIFFS_INI_H
#define SPIFFS_INI_H

#include <FS.h>
#include <SPIFFS.h>

/*
 * SPIFFSIni - A simple class for managing INI-style configuration files in SPIFFS on ESP32.
 *
 * Copyright (c) 2025 rsna6ce
 *
 * Released under the MIT License.
 * See https://github.com/rsna6ce/SPIFFSIni/blob/main/LICENSE for full license details.
 *
 * This class provides a lightweight way to read and write key-value pairs in a text file stored in SPIFFS.
 * The file format is simple: `key=value` per line, with optional comments starting with `#`.
 * It is designed for ease of use and portability, requiring no external libraries beyond the Arduino core.
 * 
 * Features:
 * - Read parameters as strings with `read()`.
 * - Check parameter existence with `exist()`.
 * - Write or update parameters with `write()`, with validation to prevent invalid keys.
 * 
 * Usage example:
 *   SPIFFSIni config("/config.ini");
 *   config.write("kp", "1.5");         // Write a parameter
 *   String value = config.read("kp");  // Read a parameter
 *   bool exists = config.exist("kp");  // Check if parameter exists
 */

class SPIFFSIni {
private:
    String file_name;  // Name of the INI file

public:
    // Constructor
    // file_name: The name of the INI file (e.g., "/config.ini")
    // format: If true, initializes SPIFFS with formatting on failure
    SPIFFSIni(String file_name, bool format = false) {
        this->file_name = file_name;
        if (format) {
            SPIFFS.begin(true);  // Initialize with formatOnFail=true
        } else {
            SPIFFS.begin();      // Normal mount
        }
    }

    // Read a parameter value
    // param_name: The name of the parameter to read
    // Returns the value as a String if found, or an empty String if not found or file doesn't exist
    String read(String param_name) {
        if (!SPIFFS.exists(file_name)) {
            return String();  // Return empty String if file doesn't exist
        }

        File file = SPIFFS.open(file_name, "r");
        if (!file) {
            return String();  // Return empty String if file can't be opened
        }

        while (file.available()) {
            String line = file.readStringUntil('\n');
            line.trim();
            if (line.startsWith("#") || line.length() == 0) {
                continue;  // Skip comment lines or empty lines
            }
            if (line.startsWith(param_name + "=")) {
                String value = line.substring(param_name.length() + 1);
                file.close();
                return value;  // Return the found value
            }
        }
        file.close();
        return String();  // Return empty String if parameter not found
    }

    // Check if a parameter exists
    // param_name: The name of the parameter to check
    // Returns true if the parameter exists in the file, false otherwise
    bool exist(String param_name) {
        if (!SPIFFS.exists(file_name)) {
            return false;  // Return false if file doesn't exist
        }

        File file = SPIFFS.open(file_name, "r");
        if (!file) {
            return false;  // Return false if file can't be opened
        }

        while (file.available()) {
            String line = file.readStringUntil('\n');
            line.trim();
            if (line.startsWith("#") || line.length() == 0) {
                continue;  // Skip comment lines or empty lines
            }
            if (line.startsWith(param_name + "=")) {
                file.close();
                return true;  // Return true if parameter is found
            }
        }
        file.close();
        return false;  // Return false if parameter not found
    }

    // Write or update a parameter
    // param_name: The name of the parameter to write
    // value: The value to write as a String
    // Returns true if successful, false if an error occurs or param_name contains '='
    bool write(String param_name, String value) {
        // Return false if param_name contains '=' (invalid parameter name)
        if (param_name.indexOf('=') != -1) {
            return false;
        }

        bool param_exists = false;
        String temp_file = file_name + ".tmp";  // Temporary file name
        File original_file;
        File temp;

        // Check if the file exists
        if (SPIFFS.exists(file_name)) {
            original_file = SPIFFS.open(file_name, "r");
            if (!original_file) {
                return false;  // Return false if original file can't be opened
            }
            temp = SPIFFS.open(temp_file, "w");
            if (!temp) {
                original_file.close();
                return false;  // Return false if temp file can't be created
            }

            // Read original file and update the parameter
            while (original_file.available()) {
                String line = original_file.readStringUntil('\n');
                line.trim();
                if (line.startsWith("#") || line.length() == 0) {
                    temp.println(line);  // Copy comments or empty lines as-is
                } else if (line.startsWith(param_name + "=")) {
                    temp.println(param_name + "=" + value);  // Update existing parameter
                    param_exists = true;
                } else {
                    temp.println(line);  // Copy other lines as-is
                }
            }
            original_file.close();
            temp.close();

            // Append if parameter wasn't found in the original file
            if (!param_exists) {
                temp = SPIFFS.open(temp_file, "a");
                if (!temp) {
                    SPIFFS.remove(temp_file);
                    return false;
                }
                temp.println(param_name + "=" + value);
                temp.close();
            }
        } else {
            // Create a new file and write the parameter
            temp = SPIFFS.open(file_name, "w");
            if (!temp) {
                return false;
            }
            temp.println(param_name + "=" + value);
            temp.close();
            return true;
        }

        // Remove original file and rename temp file
        SPIFFS.remove(file_name);
        if (!SPIFFS.rename(temp_file, file_name)) {
            SPIFFS.remove(temp_file);
            return false;
        }
        return true;
    }
};

#endif // SPIFFS_INI_H
