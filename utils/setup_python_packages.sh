python3.12 -m venv aries
source aries/bin/activate
python3.12 -m pip install --upgrade pip setuptools wheel
python3.12 -m pip install -r utils/requirements.txt
python3.12 -m pip install bfloat16