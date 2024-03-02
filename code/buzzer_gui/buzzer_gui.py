import logging
import serial
import serial.tools.list_ports

import threading
import time

import dearpygui.dearpygui as dpg

# Configure logging
logging.basicConfig(level=logging.DEBUG, format='%(asctime)s - %(levelname)s - %(message)s')

selected_port = ""  # Store the selected COM port

def find_esp32_ports():
    ports_found = {}  # Create a dictionary to store the COM port of the ESP32 device
    ports = serial.tools.list_ports.comports()
    for port in ports:
        if "Espressif" in str(port.manufacturer):
            ports_found[port.device] = port.serial_number
    return ports_found

def on_select(sender):
    global selected_port
    selected_port = dpg.get_value(sender)
    logging.info(f"Selected port: {selected_port}")

def on_button_click():
    global selected_port
    if selected_port == "":
        logging.warning("No port selected")
        return

    logging.info(f"Opening serial connection to port: {selected_port}")
    ser = serial.Serial(selected_port, 115200)

    esp32_data = ser.readline().decode('ascii')
    print(esp32_data)


def update_ports_combo():
    while True:
        time.sleep(2)  # Update every 2 seconds (adjust as needed)
        esp32_ports = find_esp32_ports()
        port_names = list(esp32_ports.keys())

        if len(esp32_ports) == 0:
            dpg.configure_item("Ports", items=[], default_value="")
            continue

        dpg.configure_item("Ports", items=port_names)


def main():
    dpg.create_context()
    dpg.create_viewport()
    dpg.setup_dearpygui()

    with dpg.window(label="Select ESP32 Port"):
        dpg.add_text("Select the COM port of the ESP32 device:")
        with dpg.group(horizontal=True):
            ports_combo = dpg.add_combo(label="Ports", tag='Ports', items=[], callback=on_select)
            dpg.add_button(label="Select", callback=on_button_click)


    dpg.show_viewport()

    # Start a thread to update the COM port list
    threading.Thread(target=update_ports_combo, daemon=True).start()

    dpg.start_dearpygui()

if __name__ == "__main__":
    main()
