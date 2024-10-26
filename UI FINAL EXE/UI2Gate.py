

######################

#pip install requests
#revisar si hay que hacer update
#C:\Users\Alber\AppData\Local\Microsoft\WindowsApps\PythonSoftwareFoundation.Python.3.10_qbz5n2kfra8p0\python.exe -m pip install --upgrade pip

import urllib.request
import json
import math
import tkinter as tk
from tkinter import font
from PIL import Image, ImageTk  # Maneja de imagenes
import requests  # Para conseguir imagen con URL
from io import BytesIO  
import webbrowser  # Abrir Maps


def fetch_aprs_data_gate(callsign, api_key):
    url = "http://api.aprs.fi/api/get?name=TiTEC3-10&what=loc&format=JSON&apikey=204772.m27dxOL5tGRsaW"
    f = urllib.request.urlopen(url)
    file =  f.read()
    json_data = json.loads(file)
    lat_g = json_data['entries'][0]['lat']
    long_g = json_data['entries'][0]['lng']
    calls_g = json_data['entries'][0]['srccall']
    return lat_g, long_g, calls_g

def fetch_aprs_data_tracker(callsign, api_key):
    url = "http://api.aprs.fi/api/get?name=TI3WTI-10&what=loc&format=JSON&apikey=204772.m27dxOL5tGRsaW"
    f = urllib.request.urlopen(url)
    file =  f.read()
    json_data = json.loads(file)
    lat_t = json_data['entries'][0]['lat']
    long_t = json_data['entries'][0]['lng']
    calls_t = json_data['entries'][0]['srccall']
    return lat_t, long_t, calls_t

def haversine(lat1, lon1, lat2, lon2):
    R = 6371  # Radius of Earth in kilometers
    dlat = math.radians(lat2 - lat1)
    dlon = math.radians(lon2 - lon1)
    a = (math.sin(dlat / 2) ** 2 +
         math.cos(math.radians(lat1)) * math.cos(math.radians(lat2)) * 
         (math.sin(dlon / 2) ** 2))
    c = 2 * math.atan2(math.sqrt(a), math.sqrt(1 - a))
    distance = R * c
    return distance


data_g = fetch_aprs_data_gate("TiTEC3-10", "204772.m27dxOL5tGRsaW")
print(data_g)


data_t = fetch_aprs_data_tracker("TI3WTI-10", "204772.m27dxOL5tGRsaW")
print(data_t)

#Converting from local variables to global variables 
lat_g, lon_g, calls_g = data_g  
lat_t, lon_t, calls_t = data_t 

distance = haversine(float(lat_t), float(lon_t), float(lat_g),float(lon_g))
print("Distance from",calls_g,"to",calls_t, "is",distance,"km")

#para boton de refrescar datos
def refresh_window():
    data_g = fetch_aprs_data_gate("TiTEC3-10", "204772.m27dxOL5tGRsaW")
    data_t = fetch_aprs_data_tracker("TI3WTI-10", "204772.m27dxOL5tGRsaW")

    value4_label.config(text="Latitud Tracker: " + str(data_t[0]))
    value5_label.config(text="Longitud Tracker: " + str(data_t[1]))
    tracker_name_label.config(text="Dispositivo: " + str(data_t[2]))

    value1_label.config(text="Latitud iGate: " + str(data_g[0]))
    value2_label.config(text="Longitud iGate: " + str(data_g[1]))
    igate_name_label.config(text="Dispositivo: " + str(data_g[2]))

    distancia_dispositivos = haversine(float(data_t[0]), float(data_t[1]), float(data_g[0]), float(data_g[1]))
    distance_value.config(text="Distancia entre ambos dispositivos: " + str(distancia_dispositivos))

#funcion para tomar coordenadas y mostrarlas en un mapa
def open_google_maps():
    igate_coords = f"{data_g[0]},{data_g[1]}"
    tracker_coords = f"{data_t[0]},{data_t[1]}"
    
    # URL para Google maps 
    map_url = f"https://www.google.com/maps/dir/{igate_coords}/{tracker_coords}"
    
    # Abrir URL
    webbrowser.open(map_url)
    

def main():
    # Ventana principal
    window = tk.Tk()
    window.title("Monitoreo de Dispositivos - Tracker y iGate")
    window.geometry("600x500")  # tamaño de la ventana

    # Definir fonts
    title_font = font.Font(family="Helvetica", size=16, weight="bold")
    label_font = font.Font(family="Helvetica", size=12)

    # Titulo principal
    title_label = tk.Label(window, text="Información de Ubicación", font=title_font, fg="blue")
    title_label.grid(row=0, column=0, columnspan=2, pady=10)

    # Organizar objetos de iGate y Tracker en un grid
    igate_frame = tk.LabelFrame(window, text="iGate", font=label_font, padx=10, pady=10)
    tracker_frame = tk.LabelFrame(window, text="Tracker", font=label_font, padx=10, pady=10)

    igate_frame.grid(row=1, column=0, padx=20, pady=10, sticky="nsew")
    tracker_frame.grid(row=1, column=1, padx=20, pady=10, sticky="nsew")

    # etiquetas iGate 
    global value1_label, value2_label, igate_name_label
    igate_name_label = tk.Label(igate_frame, text="Dispositivo: " + data_g[2], font=label_font)
    value1_label = tk.Label(igate_frame, text="Latitud iGate: " + str(data_g[0]), font=label_font)
    value2_label = tk.Label(igate_frame, text="Longitud iGate: " + str(data_g[1]), font=label_font)

    # etiquetas Tracker 
    global value4_label, value5_label, value6_label, tracker_name_label
    tracker_name_label = tk.Label(tracker_frame, text="Dispositivo: " + data_t[2], font=label_font)
    value4_label = tk.Label(tracker_frame, text="Latitud Tracker: " + str(data_t[0]), font=label_font)
    value5_label = tk.Label(tracker_frame, text="Longitud Tracker: " + str(data_t[1]), font=label_font)

    #Distancia entre ambos
    global distance_value
    distancia_dispositivos = haversine(float(data_t[0]), float(data_t[1]), float(data_g[0]), float(data_g[1]))
    distance_value = tk.Label(text="Distancia entre ambos dispositivos (km): " + str(distancia_dispositivos), font=label_font)
    
    # Place iGate labels in the iGate frame
    igate_name_label.grid(row=0, column=0, padx=10, pady=5, sticky="w")
    value1_label.grid(row=1, column=0, padx=10, pady=5, sticky="w")
    value2_label.grid(row=2, column=0, padx=10, pady=5, sticky="w")

    # poner objetos en el grid
    tracker_name_label.grid(row=0, column=0, padx=10, pady=5, sticky="w")
    value4_label.grid(row=1, column=0, padx=10, pady=5, sticky="w")
    value5_label.grid(row=2, column=0, padx=10, pady=5, sticky="w")

    # Create a button to refresh the window
    refresh_button = tk.Button(window, text="Refrescar", command=refresh_window, font=label_font, bg="lightgreen")
    refresh_button.grid(row=2, column=0, columnspan=2, pady=20)

    # Imagen
    url = "https://images.creativemarket.com/0.1.0/ps/1363862/4554/2832/m1/fpnw/wm1/world-map-blue-colors-.jpg?1465889604&s=739de0fe71a085f4f6d87f3fe01fd6a1"  
    response = requests.get(url)
    img_data = response.content
    image = Image.open(BytesIO(img_data))  
    image = image.resize((400, 100), Image.Resampling.LANCZOS)  
    photo = ImageTk.PhotoImage(image)

    image_label = tk.Label(window, image=photo)
    image_label.grid(row=3, column=0, columnspan=2, pady=10)

    
    image_label.image = photo

    # Boton para abrir mapa
    map_button = tk.Button(window, text="Abrir tracker y igate en Google Maps", command=open_google_maps, font=label_font, bg="lightblue")
    map_button.grid(row=4, column=0, columnspan=2, pady=10)

    distance_value.grid(row=5, column=0, columnspan=2, pady=10)

    # loop 
    window.mainloop()

if __name__ == "__main__":
    main()
