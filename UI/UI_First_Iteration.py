import urllib.request
import json
import tkinter as tk

##print(json_data['entries'][0]['lat'])
##print(json_data['entries'][0]['lng'])

def refval():
    link = "https://api.aprs.fi/api/get?name=TiTEC3-10&what=loc&apikey=203981.T3jsRkwKAbnQsS&format=json"
    f = urllib.request.urlopen(link)
    myfile = f.read()
    json_data = json.loads(myfile)
    lat = json_data['entries'][0]['lat']
    long = json_data['entries'][0]['lng']
    dat = json_data['entries'][0]['status']
    return lat,long,dat

def refresh_window():
    lat , long ,dat = refval()
    value1_label.config(text="Latitud: " + lat)
    value2_label.config(text="Longitud: " + long)
    value3_label.config(text="Nombre: " + dat)

def main():
    lat , long ,dat = refval()
    
    print('ok')
    # Create the main window
    window = tk.Tk()
    window.title("Valores de Longitud Latitud")
    global value1_label, value2_label, value3_label
    # Create labels to display the values
    value1_label = tk.Label(window, text="Latitud: " + lat)
    value2_label = tk.Label(window, text="Longitud: " + long)
    value3_label = tk.Label(window, text="Status: " + dat)

    # Create a button to refresh the window
    refresh_button = tk.Button(window, text="Refresh", command=refresh_window)

    # Place the labels in the window
    value1_label.pack()
    value2_label.pack()
    value3_label.pack()
    refresh_button.pack()
    
    # Start the event loop
    window.mainloop()

if __name__ == "__main__":
    main()
