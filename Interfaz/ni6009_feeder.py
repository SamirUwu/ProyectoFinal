# Existing imports
from nidaqmx import Task
from nidaqmx.constants import AcquisitionType

# Your existing code here...

# Update task timing configuration
with Task() as task:
    task.timing.cfg_samp_clk_timing(rate=args.rate, sample_mode=AcquisitionType.CONTINUOUS)

    # Set input buffer size if property is available
    if hasattr(task.in_stream, 'input_buf_size'):
        task.in_stream.input_buf_size = PACKET_SAMPLES * 4

# Rest of your code
