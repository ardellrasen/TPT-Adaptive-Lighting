   # Arduino project
   - Board FQBN: esp32:esp32:esp32
   - Port: COM5   (or /dev/ttyUSB0)
   - Compile: arduino-cli compile --fqbn esp32:esp32:esp32 .
   - Upload:  arduino-cli upload -p COM5 --fqbn esp32:esp32:esp32 .
   - Serial:  arduino-cli monitor -p COM5 -c baudrate=115200
   - Install libs with: arduino-cli lib install "<name>"
   - Always compile after editing and fix errors before reporting done.