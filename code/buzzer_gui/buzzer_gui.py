import serial
import serial.tools.list_ports
import dearpygui.dearpygui as dpg
import subprocess
import ast
from logger import mvLogger
import argparse
import time

serial_port = None  # Store the serial port object
condition_met = False  # Change this based on your actual conditions

online_bg_color = (128, 128, 128)
offline_bg_color = (50, 50, 50)
online_bg_color = (128, 128, 128)
winner_bg_color = (0, 255, 0)

buttons_ssid = []

# Define your conditions here
if condition_met:
    # Define the path to your MP4 file
    mp4_file_path = "/path/to/your/video.mp4"

    # Open the MP4 file using the default video player
    try:
        #subprocess.Popen(['xdg-open', mp4_file_path])  # For Linux
        subprocess.Popen(['open', mp4_file_path])  # For macOS
        #subprocess.Popen(['start', '', mp4_file_path], shell=True)  # For Windows
    except Exception as e:
        print(f"Error: {e}")
else:
    print("Conditions not met. Skipping opening the MP4 file.")

def log_info(message):
    global logx
    logx.log_info(message)

def log_debug(message):
    global logx
    logx.log_debug(message)

def log_warning(message):
    global logx
    logx.log_warning(message)

def log_error(message):
    global logx
    logx.log_error(message)


def find_esp32_ports():
    ports_found = {}  # Create a dictionary to store the COM port of the ESP32 device
    ports = serial.tools.list_ports.comports()
    for port in ports:
        if "Espressif" in str(port.manufacturer):
            ports_found[port.device] = port.serial_number
    return ports_found

def on_port_select(sender):
    selected_port = dpg.get_value(sender)
    log_info(f"Selected port: {selected_port}")

def on_port_select_button_click():
    global serial_port

    if serial_port is not None and serial_port.is_open:
        log_info("Serial port already opened")
        return

    selected_port = dpg.get_value("port_selection") or "(None,None)"
    # Convert the selected port from a string back to a tuple
    selected_port = ast.literal_eval(selected_port)[0] or ""

    if selected_port == "":
        log_warning("No port selected")
        return

    dpg.configure_item("reset_button", enabled=True)
    dpg.configure_item("scan_button", enabled=True)
    dpg.configure_item("port_close_button", enabled=True)
    log_info("Reset button enabled")
    log_info("Scan button enabled")
    log_info("Port close button enabled")

    log_info(f"Opening serial connection to port: {selected_port}")

    serial_port = serial.Serial(selected_port, 115200)

    if serial_port.is_open:
        log_info(f"Serial port opened: {selected_port}")
    else:
        log_error(f"Failed to open serial port: {selected_port}")


def update_ports_combo():
    esp32_ports = find_esp32_ports()
    port_names = list(esp32_ports.items())
    dpg.configure_item("port_selection", items=port_names)

def update_button_status():
    # Check if serial port exist and is open
    if not (serial_port is not None and serial_port.is_open):
        return

    if serial_port.in_waiting == 0:
        return

    esp32_data = serial_port.readline().decode('utf-8').strip("\n")
    log_debug(f"Data received from ESP32: {esp32_data}")

    # Message format: Slave SSID: 30:AE:A4:1A:2B:3C, length: 30
    if "Slave SSID:" in esp32_data[0:13] and len(esp32_data) == 30:
        esp32_data = esp32_data[12:]
        if esp32_data not in buttons_ssid and len(buttons_ssid) < 5:
            log_info(f"Adding button with SSID: {esp32_data}")
            dpg.configure_item(f'button_ssid_{len(buttons_ssid)+1}', text=f"SSID:{esp32_data}")
            dpg.configure_item(f'button_shape_{len(buttons_ssid)+1}', color=online_bg_color, fill=online_bg_color)
            buttons_ssid.append(esp32_data)

    if "Winner Mac:" in esp32_data[0:13] and len(esp32_data) == 30:
        esp32_data = esp32_data[12:]
        if esp32_data in buttons_ssid:
            log_info(f"Winner button with SSID: {esp32_data}")
            dpg.configure_item(f'button_shape_{buttons_ssid.index(esp32_data)+1}', color=winner_bg_color, fill=winner_bg_color)

def get_rssi():
    if serial_port is not None and serial_port.is_open:
        serial_port.write(b'w')


def on_ping_button_clicked(sender):
    id = int(sender[-1])

    if dpg.is_item_hovered(sender):
        if id > len(buttons_ssid):
            log_warning(f"Button {id} does not exist")
            return
        if serial_port is not None and serial_port.is_open:
            serial_port.write(f"ping {buttons_ssid[id-1]}\n".encode('utf-8'))
            # get_ssid()
            serial_port.readline().decode('ascii')
            log_info(f"Pinging button {id}")
            dpg.configure_item(f'button_shape_{id}', color=winner_bg_color, fill=winner_bg_color)
        else:
            log_warning(f"Serial port not open, unable to ping button {id}")

def on_reset_click():
    log_info("Resetting game")
    for i in range(1, len(buttons_ssid)+1):
        dpg.configure_item(f'button_shape_{i}', color=online_bg_color, fill=online_bg_color)

def on_scan_click(sender, app_data, user_data):
    state = user_data
    state = not state
    log_info(f"Scan button state: {state}")

    dpg.set_item_user_data(sender, state)


def scanning_buttons():
    state = dpg.get_item_user_data("scan_button")

    if not (serial_port is not None and serial_port.is_open):
        return

    if not state:
        return

    serial_port.write(b's')
    log_info("Scanning for buttons")

    esp32_data = serial_port.readline().decode('ascii')
    if "Slave:" in esp32_data[0:7] and len(esp32_data) == 25:
        esp32_data = esp32_data[6:]
        if esp32_data not in buttons_ssid and len(buttons_ssid) < 5:
            log_info(f"Adding button with SSID: {esp32_data}")
            dpg.configure_item(f'button_ssid_{len(buttons_ssid)+1}', text=f"SSID:{esp32_data}")
            dpg.configure_item(f'button_shape_{len(buttons_ssid)+1}', color=online_bg_color, fill=online_bg_color)
            buttons_ssid.append(esp32_data)



def on_close_click():
    global serial_port
    if serial_port is not None and serial_port.is_open:
        serial_port.close()
        log_info("Serial port closed")
    dpg.configure_item("reset_button", enabled=False)
    dpg.configure_item("scan_button", enabled=False)
    dpg.configure_item("port_close_button", enabled=False)
    log_info("Reset button disabled")
    log_info("Scan button disabled")
    log_info("Port close button disabled")

def main(log_level):
    global logx, serial_port

    text_color = (255, 255, 255)
    text_size = 15

    dpg.create_context()
    dpg.create_viewport(title='MT24 Buzzer', width=1280, height=1080, x_pos=2000, y_pos=100)
    dpg.setup_dearpygui()

    # with dpg.window(label="MT24 Buzzer", width=1250, height=750):
    with dpg.window(label="COM", no_close=True, width=500, height=130, pos=(0, 0)):
        dpg.add_text("Select the COM port of the ESP32 device:")
        with dpg.group(horizontal=True):
            dpg.add_combo(label="Ports", tag='port_selection', items=[], callback=on_port_select)
            dpg.add_button(label="Select", tag='port_select_button', callback=on_port_select_button_click)
            dpg.add_button(label="Close", tag='port_close_button', callback=on_close_click, enabled=False)

        with dpg.group(horizontal=True):
            dpg.add_button(label="RESET", tag='reset_button', callback=on_reset_click, enabled=False, width=100, height=50)
            dpg.add_button(label="SCAN", tag='scan_button', callback=on_scan_click, enabled=False, width=100, height=50, user_data=False)


    with dpg.window(label="Buzzer Status", tag="buzzer_list", pos=(0, 150), width=1280, height=300, no_close=True):
        with dpg.group(horizontal=True, horizontal_spacing=10):
            for i in range(1,6):
                with dpg.drawlist(width=240, height=200, pos=(200*(i), 0), tag=f'button_drawlist_{i}', callback=on_ping_button_clicked):
                    x, y = 10, 20
                    dpg.draw_rectangle((0, 0), (240, 200), tag=f'button_shape_{i}', color=offline_bg_color, fill=offline_bg_color)
                    dpg.draw_text((50, y), f"Button {i}", color=text_color, size=text_size, tag=f'button_text_{i}')
                    dpg.draw_text((x, y+30), "Status:", color=text_color, size=text_size, tag=f'button_status_{i}')
                    dpg.draw_text((x, y+60), "SSID:", color=text_color, size=text_size, tag=f'button_ssid_{i}')
                    dpg.draw_text((x, y+90), "RSSI:", color=text_color, size=text_size, tag=f'button_rssi_{i}')
                    dpg.draw_text((x, y+120), "Battery:", color=text_color, size=text_size, tag=f'button_battery_{i}')


    with dpg.window(label="Logging", tag="log", pos=(0, 470), width=1280, height=300, no_close=True):
        logx = mvLogger(parent="log")
        logx.log_level = log_level

    dpg.show_viewport()

    # below replaces, start_dearpygui()
    while dpg.is_dearpygui_running():
        # insert here any code you would like to run in the render loop
        update_ports_combo()
        update_button_status()
        scanning_buttons()
        dpg.render_dearpygui_frame()

    dpg.destroy_context()

    # dpg.start_dearpygui()

if __name__ == "__main__":
    args = argparse.ArgumentParser(description="MT24 Buzzer GUI")
    args.add_argument("--log_level", type=int, default=2, help="Log level")
    args = args.parse_args()
    main(args.log_level)
