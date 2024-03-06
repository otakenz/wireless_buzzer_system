import logging
import serial
import serial.tools.list_ports
import threading
import time
import dearpygui.dearpygui as dpg
import subprocess

# Configure logging
logging.basicConfig(level=logging.DEBUG, format='%(asctime)s - %(levelname)s - %(message)s')

selected_port = ""  # Store the selected COM port
reset_button_id = None  # Store the ID of the reset button
log_text_id = None  # Store the ID of the log text

# Define your conditions here
condition_met = False  # Change this based on your actual conditions

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

def log_message(message):
    global log_text_id
    current_log = dpg.get_value(log_text_id)
    updated_log = current_log + message + "\n"
    dpg.set_value(log_text_id, updated_log)

def find_esp32_ports():
    ports_found = {}  # Create a dictionary to store the COM port of the ESP32 device
    ports = serial.tools.list_ports.comports()
    for port in ports:
        if "Espressif" in str(port.manufacturer):
            ports_found[port.device] = port.serial_number
    return ports_found

def on_select(sender):
    global selected_port, reset_button_id
    selected_port = dpg.get_value(sender)
    log_message(f"Selected port: {selected_port}")
    if reset_button_id is not None:
        dpg.configure_item(reset_button_id, enabled=bool(selected_port))

def on_port_select_button_click():
    global selected_port
    if selected_port == "":
        log_message("No port selected")
        return

    log_message(f"Opening serial connection to port: {selected_port}")
    ser = serial.Serial(selected_port, 115200)

    esp32_data = ser.readline().decode('ascii')
    print(esp32_data)

def on_reset_click():
    global selected_port, reset_button_id
    selected_port = ""
    log_message("Port selection reset")
    if reset_button_id is not None:
        dpg.configure_item(reset_button_id, enabled=False)
    reset_gui_inputs()  # Reset all GUI inputs
    reset_table_cells()
#    clear_table_cell(0,2)

def reset_gui_inputs():
    # Reset all GUI inputs here
    dpg.set_value("Ports", "")  # Reset port selection
    # Add more GUI items to reset as needed

def clear_table_cell(row_index, column_index):
    item_id = f"TableItem{row_index}_{column_index}"
    dpg.set_value(item_id, "")

def reset_table_cells():
    # Reset table cells here
    dpg.delete_item("Table", children_only=True)

    with dpg.table("Table", header_row=True, row_background=False,
                   borders_innerH=True, borders_outerH=True, borders_innerV=True,
                   borders_outerV=True):
        
        # Re-populate the table with the default structure
        dpg.add_table_column(label="Labels")
        dpg.add_table_column(label="Button 1")
        dpg.add_table_column(label="Button 2")
        dpg.add_table_column(label="Button 3")
        dpg.add_table_column(label="Button 4")
        dpg.add_table_column(label="Button 5")

        for i in range(0, 6):  # row
            with dpg.table_row():
                for j in range(0, 6):  # column
                    # print labels
                    if j == 0:
                        if i == 0:
                            dpg.add_text(f"Fastest button")
                        elif i == 1:
                            dpg.add_text(f"MAC Address")
                        elif i == 2:
                            dpg.add_text(f"RSSI")
                        elif i == 3:
                            dpg.add_text(f"Battery")
                        elif i == 4:
                            dpg.add_text(f"")
                        elif i == 5:
                            dpg.add_text(f"")
                    if j==3 and i==2:
                        dpg.add_text(f"RSSI", parent=table_id)
 #                   else:
 #                       if i == 0:
 #                           # add winning conditions here
 #                           if j == 1:
 #                               dpg.add_text(f"FASTEST", color=(0, 255, 0, 255))
 #                           # add losing conditions here
 #                           else:
 #                               dpg.add_text(f"SLOW", color=(255, 0, 0, 255))
 #                       else:
 #                           dpg.add_text("")

def update_ports_combo():
    global reset_button_id
    while True:
        time.sleep(2)  # Update every 2 seconds (adjust as needed)
        esp32_ports = find_esp32_ports()
        port_names = list(esp32_ports.keys())

        if len(esp32_ports) == 0:
            dpg.configure_item("Ports", items=[], default_value="")
            if reset_button_id is not None:
                dpg.configure_item(reset_button_id, enabled=False)
            continue

        dpg.configure_item("Ports", items=port_names)
        if reset_button_id is not None:
            dpg.configure_item(reset_button_id, enabled=bool(selected_port))

def buttons_status_table():
    with dpg.table(label="Table", header_row=True, row_background=False,
                borders_innerH=True, borders_outerH=True, borders_innerV=True,
                borders_outerV=True):

        # use add_table_column to add columns to the table, 6x6
        dpg.add_table_column(label="Labels")
        dpg.add_table_column(label="Button 1")
        dpg.add_table_column(label="Button 2")
        dpg.add_table_column(label="Button 3")
        dpg.add_table_column(label="Button 4")
        dpg.add_table_column(label="Button 5")

        # printing table contents
        for i in range(0, 6): # row
            with dpg.table_row():
                for j in range(0, 6): # column
                    # print labels
                    if j==0:
                        if i==0:
                            dpg.add_text(f"Fastest button")
                        elif i==1:
                            dpg.add_text(f"MAC Address")
                        elif i==2:
                            dpg.add_text(f"RSSI")
                        elif i==3:
                            dpg.add_text(f"Battery")
                        elif i==4:
                            dpg.add_text(f"")
                        elif i==5:
                            dpg.add_text(f"")
                        continue
                    
                    # print details
                    # print fastest buzzer                    
                    else:
                        if i==0:
                        # add winning conditions here
                            if j==1:
                                dpg.add_text(f"FASTEST", color = (0, 255, 0, 255))
                            # add losing conditions here
                            else:
                                dpg.add_text(f"SLOW", color = (255, 0, 0, 255))
                        continue
        for i in range(0, 6): # row
            with dpg.table_row():
                for j in range(0, 6): # column
                    item = f"TableItem{i}_{j}"
                    dpg.add_text(label="", id=item)
    
def main():
    global reset_button_id, log_text_id
    dpg.create_context()
    dpg.create_viewport()
    dpg.setup_dearpygui()

    with dpg.window(label="MT24 Buzzer", width=1250, height=750):
        dpg.add_text("Select the COM port of the ESP32 device:")
        with dpg.group(horizontal=True):
            ports_combo = dpg.add_combo(label="Ports", tag='Ports', items=[], callback=on_select)
            dpg.add_button(label="Select", callback=on_port_select_button_click)

        with dpg.group(horizontal=True):
            reset_button_id = dpg.add_button(label="RESET", callback=on_reset_click, enabled=False, width=100, height=50)

        buttons_status_table()

        # Create a child container for the real-time logging
        with dpg.child(width=400, height=300):
            with dpg.group(horizontal=True):
                log_text_id = dpg.add_text("")
                dpg.add_same_line()

    dpg.show_viewport()

    # Start a thread to update the COM port list
    threading.Thread(target=update_ports_combo, daemon=True).start()

    dpg.start_dearpygui()

if __name__ == "__main__":
    main()
