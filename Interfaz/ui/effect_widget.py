from PyQt6.QtWidgets import QWidget, QVBoxLayout, QPushButton, QLabel, QSlider, QListWidgetItem 
from PyQt6.QtCore import Qt

class EffectWidget(QWidget):
    def __init__(self, effect_data, update_callback):
        super().__init__()

        self.list_item = None
        
        self.effect_data = effect_data
        self.update_callback = update_callback
        self.expanded = False

        self.main_layout = QVBoxLayout()
        self.setLayout(self.main_layout)
        
        #Header
        self.header_button = QPushButton(effect_data["type"] + " ▼")
        self.header_button.setCheckable(True)
        self.header_button.clicked.connect(self.toggle_expand)
        self.main_layout.addWidget(self.header_button)

        #Parametros
        self.params_Widget = QWidget()
        self.params_Layout = QVBoxLayout()
        self.params_Widget.setLayout(self.params_Layout)
        self.sliders = {}

        #Sliders
        for param, value in effect_data["params"].items():
            label = QLabel(f"{param}: {value}")
            slider = QSlider(Qt.Orientation.Horizontal)
            slider.setMinimum(0)
            slider.setMaximum(100)
            slider.setValue(int(value * 100))

            slider.valueChanged.connect(
                lambda val, p=param, l=label: self.update_param(p, val, l)
                
            )
            self.params_Layout.addWidget(label)
            self.params_Layout.addWidget(slider)
            self.sliders[param] = slider
        
        self.main_layout.addWidget(self.params_Widget)
        self.params_Widget.setVisible(False)

    def toggle_expand(self):
        self.expanded = not self.expanded
        self.params_Widget.setVisible(self.expanded)

        arrow = "▲" if self.expanded else "▼"
        self.header_button.setText(self.effect_data["type"] + " " + arrow)

        if hasattr(self, "list_item"):
            self.list_item.setSizeHint(self.sizeHint())

    def update_param(self, param, value, label):
        normalized = value / 100
        self.effect_data["params"][param] = normalized
        label.setText(f"{param}: {round(normalized, 2)}") 
        print("update_param called")
        self.update_callback()