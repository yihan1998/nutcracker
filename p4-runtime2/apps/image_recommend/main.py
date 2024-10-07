import os
from PIL import Image
import numpy as np
import zlib
import time
import joblib
import traceback
import struct
from sklearn.naive_bayes import MultinomialNB
from sklearn.preprocessing import MinMaxScaler
from sklearn.datasets import fetch_openml
from skimage import color

path = '~/Nutcracker/utils/'

# Load the Multinomial Naive Bayes model and the pre-fitted MinMaxScaler
model_filename = os.path.expanduser(path + 'mnb_cifar10.joblib')
scaler_filename = os.path.expanduser(path + 'scaler_cifar10.joblib')
model = joblib.load(model_filename)
scaler = joblib.load(scaler_filename)

# Load the KV store from the saved file
kv_store_filename = os.path.expanduser(path + 'cifar10_kv_store.joblib')
kv_store = joblib.load(kv_store_filename)

def main():
    print("Hello, World!")