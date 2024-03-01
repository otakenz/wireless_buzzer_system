# import serial

# # Open the serial port connected to the ESP32-C3
# ser = serial.Serial('/dev/ttyACM0', 115200)  # Change '/dev/ttyUSB0' to the correct serial port on your system

# # Function to send data to the ESP32-C3
# def getValue_on():
#     ser.write(b'on')
#     # esp32_data = ser.readline().decode().strip()
#     esp32_data = ser.readline().decode('ascii')
#     return esp32_data


# def getValue_off():
#     ser.write(b'off')
#     esp32_data = ser.readline().decode('ascii')
#     return esp32_data


# while(1):
#     input_value = input("Enter on or off: ")
#     if input_value == 'on':
#         print(getValue_on())
#     elif input_value == 'off':
#         print(getValue_off())
#     else:
#         print("Invalid input")

import serial
import serial.tools.list_ports


import dearpygui.dearpygui as dpg

ports_found = {}  # Create a dictionary to store the COM port of the ESP32 device
def find_esp32_ports():
    ports = serial.tools.list_ports.comports()
    for port in ports:
        if "Espressif" in str(port.manufacturer):
            ports_found[port.device] = port.serial_number
    return ports_found

def on_select(sender):
    global selected_port
    selected_port = dpg.get_value(sender)
    print(f"Selected port: {selected_port}")

def on_button_click(sender):
    global selected_port
    print(f"Opening port: {selected_port}")
    ser = serial.Serial(selected_port, 115200)

    esp32_data = ser.readline().decode('ascii')
    print(esp32_data)


ports = find_esp32_ports()
port_names = list(ports.keys())
# port_names = [f"{port} ({serial})" for port, serial in ports.items()]

def main():
    dpg.create_context()
    dpg.create_viewport()
    dpg.setup_dearpygui()


    with dpg.window(label="Select ESP32 Port"):
        dpg.add_text("Select the COM port of the ESP32 device:")
        with dpg.group(horizontal=True):
            dpg.add_combo(label="Ports", items=port_names, callback=on_select)
            dpg.add_button(label="Select", callback=on_button_click)

    dpg.show_viewport()
    dpg.start_dearpygui()

if __name__ == "__main__":
    main()
