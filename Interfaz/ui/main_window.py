import sys
import numpy as np
import pyqtgraph as pg
import json
import wave
import struct

from ui.effect_widget import EffectWidget
from core.preset_model import PresetModel
from server.receiver_app import TcpServer
from server.receiver_c import SocketReceiver

from collections import deque
from PyQt6.QtWidgets import QApplication, QWidget, QVBoxLayout, QHBoxLayout, QPushButton, QLabel, QComboBox, QListWidget, QListWidgetItem 
from PyQt6.QtCore import QTimer, Qt

class MainWindow(QWidget):
    SAMPLE_RATE = 44100  

    def __init__(self):
        super().__init__()

        self.setWindowTitle("Audio Interface")

        # Conexión con C
        self.receiver = SocketReceiver()
        self.receiver.pre_received.connect(self.update_pre_buffer)
        self.receiver.post_received.connect(self.update_buffer)
        self.receiver.start()

        self.pre_buffer = deque([0]*500, maxlen=44100*5)
        self.signal_buffer = deque([0]*500, maxlen=44100*5)

        # Layouts
        self.main_layout = QHBoxLayout()
        self.setLayout(self.main_layout)
        self.left_layout = QVBoxLayout()
        self.right_layout = QVBoxLayout()
        self.main_layout.addLayout(self.left_layout, 1)
        self.main_layout.addLayout(self.right_layout, 2)

        # Título
        self.title_label = QLabel("MultiFX Processor")
        self.left_layout.addWidget(self.title_label)

        # Dropdown
        self.preset_dropdown = QComboBox()
        self.preset_dropdown.addItems(["Preset 1", "Preset 2", "Preset 3"])
        self.left_layout.addWidget(self.preset_dropdown)

        # Lista de efectos
        self.effects_list = QListWidget()
        self.effects_list.setDragDropMode(QListWidget.DragDropMode.InternalMove)
        self.effects_list.model().rowsMoved.connect(self.update_effect_order)
        self.left_layout.addWidget(self.effects_list)

        self.model = PresetModel("Preset1")
        initial_effects = [
            {"id": "fx_1", "type": "Distortion", "enabled": True,
             "params": {"GAIN": 0.5, "TONE": 0.5, "OUTPUT": 0.5}},
            {"id": "fx_2", "type": "Delay", "enabled": True,
             "params": {"TIME": 0.5, "FEEDBACK": 0.3, "MIX": 0.2}},
            {"id": "fx_3", "type": "Wah", "enabled": True,
             "params": {"FREQ": 0.5, "Q": 0.3, "LEVEL": 0.2}},
            {"id": "fx_4", "type": "Flanger", "enabled": True,
             "params": {"RATE": 0.5, "DEPTH": 0.3, "FEEDBACK": 0.2, "MIX": 0.5}},
        ]
        self.model.set_effects(initial_effects)
        self.load_effects()

        # Botones
        self.toggle_fft_btn = QPushButton("Show FFT")
        self.toggle_fft_btn.setCheckable(True)
        self.toggle_fft_btn.clicked.connect(self.toggle_fft)
        self.right_layout.addWidget(self.toggle_fft_btn)

        # Guardar WAV
        desktop_path = "/mnt/c/Users/hp/Desktop/my_signal.wav"
        self.save_wav_btn = QPushButton("Save WAV")
        self.save_wav_btn.clicked.connect(lambda: self.save_wav(desktop_path))
        self.right_layout.addWidget(self.save_wav_btn)

        # Estado inicial
        self.show_fft = False

        # Plots
        self.plot_pre = pg.PlotWidget(title="Pre Effect Signal")
        self.plot_pre.setLabel("left", "Amplitude")
        self.plot_pre.setLabel("bottom", "Time")
        self.curve_pre = self.plot_pre.plot()
        self.right_layout.addWidget(self.plot_pre)

        self.plot_post = pg.PlotWidget(title="Post Effect Signal")
        self.plot_post.setLabel("left", "Amplitude")
        self.plot_post.setLabel("bottom", "Time")
        self.curve_post = self.plot_post.plot()
        self.right_layout.addWidget(self.plot_post)

        # Timer
        self.timer = QTimer()
        self.timer.timeout.connect(self.sim_signal)
        self.timer.start(150)

        # Servidor TCP
        self.server = TcpServer()
        self.server.json_received.connect(self.handle_remote_json)
        self.server.start()

    # --- Métodos ---
    def update_effect_order(self, *args):
        new_order = []
        for i in range(self.effects_list.count()):
            item = self.effects_list.item(i)
            widget = self.effects_list.itemWidget(item)
            new_order.append(widget.effect_data)
        self.model.update_order(new_order)
        print(self.model.to_json())

    def handle_remote_json(self,data):
        self.model.load_from_json(data)
        self.load_effects()
        print("Nuevo estado: ", self.model.to_json())

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

    def handle_param_change(self, effect_id, param, value):
        self.model.update_param(effect_id, param, value)
        print("JSON ready for C++:", self.model.to_json())

    def save_wav(self, filename):
        N = min(len(self.signal_buffer), self.SAMPLE_RATE*5)
        y = list(self.signal_buffer)[-N:]
        max_val = max(abs(np.min(y)), abs(np.max(y)), 1e-12)
        y_norm = [v / max_val for v in y]
        y_int16 = [int(v * 32767) for v in y_norm]

        with wave.open(filename, 'w') as wf:
            wf.setnchannels(1)
            wf.setsampwidth(2)
            wf.setframerate(self.SAMPLE_RATE)
            wf.writeframes(struct.pack('<' + 'h'*len(y_int16), *y_int16))
        print(f"WAV guardado como {filename}")

    def update_buffer(self, value):
        self.signal_buffer.append(value)

    def update_pre_buffer(self, value):
        self.pre_buffer.append(value)

    def sim_signal(self):
        x_pre = np.arange(len(self.pre_buffer))
        x_post = np.arange(len(self.signal_buffer))
        if not self.show_fft:
            self.plot_post.setLabel("bottom", "Time")
            self.plot_post.setLabel("left", "Amplitude")
            self.curve_pre.setData(x_pre, list(self.pre_buffer))
            self.curve_post.setData(x_post, list(self.signal_buffer))
        else:
            y = np.array(self.signal_buffer, dtype=float)
            N_fft = 4096
            if len(y) < N_fft:
                y = np.pad(y, (0, N_fft - len(y)), 'constant')
            window = np.hanning(len(y))
            y_win = y * window
            Y = np.fft.rfft(y_win)
            Y_mag_db = 20 * np.log10(np.abs(Y) / len(Y) + 1e-12)
            freqs = np.fft.rfftfreq(N_fft, d=1.0/self.SAMPLE_RATE)
            self.curve_post.setData(freqs, Y_mag_db)
            self.curve_pre.setData(x_pre, list(self.pre_buffer))

    def toggle_fft(self):
        self.show_fft = self.toggle_fft_btn.isChecked()
        self.toggle_fft_btn.setText("Show Time" if self.show_fft else "Show FFT")