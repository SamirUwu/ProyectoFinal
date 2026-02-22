import sys
import numpy as np
import pyqtgraph as pg
import json
from PyQt6.QtWidgets import QApplication, QWidget, QVBoxLayout, QHBoxLayout, QPushButton, QSlider, QLabel, QComboBox, QListWidget
from PyQt6.QtCore import QTimer, Qt


class MainWindow(QWidget):

    def __init__(self):
        super().__init__()

        self.setWindowTitle("Audio Interface")

        self.t = 0
        self.effect_on = False
        self.drive = 1
        
        #Definimos el tipo de layer
        self.main_layout = QHBoxLayout()
        self.setLayout(self.main_layout)

        self.left_layout = QVBoxLayout()
        self.right_layout = QVBoxLayout()

        self.main_layout.addLayout(self.left_layout, 1)
        self.main_layout.addLayout(self.right_layout, 2)

        #Boton
        self.button = QPushButton("Toggle Effect")
        self.left_layout.addWidget(self.button)

        self.button.clicked.connect(self.toggle_effect)

        #Labels
        self.drive_label = QLabel("Drive: 1")
        self.left_layout.addWidget(self.drive_label)

        #Slider
        self.drive_slider = QSlider(Qt.Orientation.Horizontal)
        self.drive_slider.setMinimum(1)
        self.drive_slider.setMaximum(20)    
        self.drive_slider.setValue(1)
        self.left_layout.addWidget(self.drive_slider)

        self.drive_slider.valueChanged.connect(self.update_drive)

        #Plot de las señales
        self.plot_pre = pg.PlotWidget(tittle="Pre Effect Signal")
        self.plot_pre.setTitle("Pre Effect", size="12pt")
        self.plot_pre.setLabel("left", "Amplitude")
        self.plot_pre.setLabel("bottom", "Time")
        self.curve_pre = self.plot_pre.plot()
        self.right_layout.addWidget(self.plot_pre)

        self.plot_post = pg.PlotWidget(tittle="Post Effect Signal")
        self.plot_post.setTitle("Post Effect", size="12pt")
        self.plot_post.setLabel("left", "Amplitude")
        self.plot_post.setLabel("bottom", "Time")
        self.curve_post = self.plot_post.plot()
        self.right_layout.addWidget(self.plot_post)

        self.timer = QTimer()
        self.timer.timeout.connect(self.sim_signal)
        self.timer.start(80) #Elegir velocidad en la que se generan los puntos

        self.effects = [
            {"type": "Distortion", "enabled": True, "params": {"GAIN": 0.5, "TONE": 0.5, "OUTPUT": 0.5}},
            {"type": "Delay", "enabled": True, "params": {"TIME": 120.0, "FEEDBACK": 0.3, "MIX": 0.2}},
            {"type": "Wah", "enabled": True, "params": {}},
            {"type": "Flanger", "enabled": True, "params": {}},
        ]
        self.preset_name = "Preset1"

    #Presionar efecto
    def toggle_effect(self):
        self.effect_on = not self.effect_on
        self.effects[0]["enabled"] = self.effect_on
        print("Effect:", self.effect_on)
        self.generate_json()

    def generate_json(self):
        payload = {
            "command": "apply_preset",
            "name": self.preset_name,
            "effects": self.effects   
        }
        print("JSON ready for C++: ")
        print(json.dumps(payload, indent=2))

    #Print de valor del Drive
    def update_drive(self, value):
        self.drive = value
        self.drive_label.setText(f"Drive: {value}")

    #Definir funciones
    def sim_signal(self):
        x = np.linspace(self.t, self.t + 1, 500)
        
        clean_sig = np.sin(2 * np.pi * 5 * x)

        if self.effect_on:
            processed_sig = np.tanh(clean_sig * self.drive)
        else:
            processed_sig = clean_sig.copy()

        self.curve_pre.setData(clean_sig)
        self.curve_post.setData(processed_sig)

        self.t += 0.05


app = QApplication(sys.argv)
window = MainWindow()
window.show()
sys.exit(app.exec())