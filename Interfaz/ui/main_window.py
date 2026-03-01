import sys
import numpy as np
import pyqtgraph as pg
import json

from ui.effect_widget import EffectWidget
from core.preset_model import PresetModel

from PyQt6.QtWidgets import QApplication, QWidget, QVBoxLayout, QHBoxLayout, QPushButton, QSlider, QLabel, QComboBox, QListWidget, QListWidgetItem 
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

        #Titulo
        self.title_label = QLabel("MultiFX Processor")
        self.left_layout.addWidget(self.title_label)
        
        #Dropdown
        self.preset_dropdown = QComboBox()
        self.preset_dropdown.addItems(["Preset 1", "Preset 2", "Preset 3"])
        self.left_layout.addWidget(self.preset_dropdown)
        
        #Lista de efectos
        self.effects_list = QListWidget()
        self.effects_list.setDragDropMode(QListWidget.DragDropMode.InternalMove)
        self.effects_list.model().rowsMoved.connect(self.update_effect_order)
        self.model = PresetModel("Preset1")

        initial_effects = [
            {"id": "fx_1", "type": "Distortion", "enabled": True,
            "params": {"GAIN": 0.5, "TONE": 0.5, "OUTPUT": 0.5}},

            {"id": "fx_2", "type": "Delay", "enabled": True,
            "params": {"TIME": 0.5, "FEEDBACK": 0.3, "MIX": 0.2}},

            {"id": "fx_3", "type": "Wah", "enabled": True,
            "params": {}},

            {"id": "fx_4", "type": "Flanger", "enabled": True,
            "params": {}},
        ]

        self.model.set_effects(initial_effects)

        #Cargar efectos
        self.load_effects()
        self.left_layout.addWidget(self.effects_list)
        
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
        self.plot_pre = pg.PlotWidget(title="Pre Effect Signal")
        self.plot_pre.setTitle("Pre Effect", size="12pt")
        self.plot_pre.setLabel("left", "Amplitude")
        self.plot_pre.setLabel("bottom", "Time")
        self.curve_pre = self.plot_pre.plot()
        self.right_layout.addWidget(self.plot_pre)

        self.plot_post = pg.PlotWidget(title="Post Effect Signal")
        self.plot_post.setTitle("Post Effect", size="12pt")
        self.plot_post.setLabel("left", "Amplitude")
        self.plot_post.setLabel("bottom", "Time")
        self.curve_post = self.plot_post.plot()
        self.right_layout.addWidget(self.plot_post)

        self.timer = QTimer()
        self.timer.timeout.connect(self.sim_signal)
        self.timer.start(80) #Elegir velocidad en la que se generan los puntos

        
    def update_effect_order(self, *args):
        new_order = []

        for i in range(self.effects_list.count()):
            item = self.effects_list.item(i)
            widget = self.effects_list.itemWidget(item)
            new_order.append(widget.effect_data)

        self.model.update_order(new_order)
        print(self.model.to_json())
        
    #Presionar efecto
    def toggle_effect(self):
        self.effect_on = not self.effect_on
        self.effects[0]["enabled"] = self.effect_on
        print("Effect:", self.effect_on)
        self.generate_json()
    
    #Cargar efectos
    def load_effects(self):
        self.effects_list.clear()
        
        for effect in self.model.effects:
            item = QListWidgetItem()
            widget = EffectWidget(effect)
            widget.list_item = item
            
            item.setSizeHint(widget.sizeHint())

            widget.param_changed.connect(self.handle_param_change)
            self.effects_list.addItem(item)
            self.effects_list.setItemWidget(item, widget)
    
    #Generación del Json
    def generate_json(self):
        print("JSON ready for C++: ")
        print(self.model.to_json())

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

    def handle_param_change(self, effect_id, param, value):
        print("MainWindow updating model")

        self.model.update_param(effect_id, param, value)

        print("JSON ready for C++:")
        print(self.model.to_json())