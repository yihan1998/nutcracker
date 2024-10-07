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

def preprocess_image(image):
    """Preprocess image for Multinomial Naive Bayes."""
    image = image.resize((32, 32))  # CIFAR-10 images are 32x32
    image_array = np.array(image).astype('float32') / 255
    image_array = image_array.reshape(1, -1)  # Flatten the image
    image_array = scaler.transform(image_array)  # Apply Min-Max scaling
    return image_array

def classify_image(image):
    """Classify the input image."""
    try:
        start_preprocess = time.time()
        image_array = preprocess_image(image)
        end_preprocess = time.time()
        
        start_predict = time.time()
        predictions = model.predict(image_array)
        predicted_class = predictions[0]
        end_predict = time.time()

        preprocess_time = (end_preprocess - start_preprocess) * 1e9  # Convert to nanoseconds
        predict_time = (end_predict - start_predict) * 1e9  # Convert to nanoseconds
        
        return predicted_class, preprocess_time, predict_time
    except Exception as e:
        print("Error during classification:")
        traceback.print_exc()
        return None, 0, 0

def recommend(predicted_class, n_recommendations=5):
    """Recommend images of the same class as the predicted class."""
    try:
        start_time = time.time()
        class_images = kv_store[predicted_class]
        recommended_indices = np.random.choice(len(class_images), n_recommendations, replace=False)
        recommended_images = [class_images[i] for i in recommended_indices]
        recommendation_time = (time.time() - start_time) * 1e9  # Convert to nanoseconds
        return recommended_images, recommendation_time
    except Exception as e:
        print("Error during recommendation:")
        traceback.print_exc()
        return [], 0

def recommend(predicted_class, n_recommendations=5):
    """Recommend images of the same class as the predicted class."""
    try:
        start_time = time.time()
        class_images = kv_store[predicted_class]
        recommended_indices = np.random.choice(len(class_images), n_recommendations, replace=False)
        
        recommended_images = [class_images[i] for i in recommended_indices]
        recommendation_time = (time.time() - start_time) * 1e9  # Convert to nanoseconds
        return recommended_images, recommendation_time
    except Exception as e:
        print("Error during recommendation:")
        traceback.print_exc()
        return [], 0

def compress_image(image):
    """Compress an image using zlib deflate."""
    try:
        start_compress = time.time()
        image_bytes = image.tobytes()  # Convert image to bytes
        compressed_data = zlib.compress(image_bytes)
        compression_time = (time.time() - start_compress) * 1e9  # Convert to nanoseconds
        return compressed_data, compression_time
    except Exception as e:
        print("Error during compression:")
        traceback.print_exc()
        return None, 0

def image_recommend(data):
    try:
        image = Image.fromarray(np.frombuffer(data, dtype=np.uint8).reshape((32, 32, 3)), 'RGB')
    except Exception as img_err:
        print("Error converting byte array to image:")
        traceback.print_exc()
        return 1
    
    # Classify the image
    predicted_class, preprocess_time, predict_time = classify_image(image)
    if predicted_class is None:
        return 1

    # Recommend images
    recommended_images, recommendation_time = recommend(predicted_class)

    # Compress recommended images
    total_compression_time = 0    
    # Compress the original image
    original_compressed_data, original_compression_time = compress_image(image)
    for img in recommended_images:
        _, compression_time = compress_image(Image.fromarray(img))
        total_compression_time += compression_time
    
    # Calculate the total processing time and percentages
    total_time = preprocess_time + predict_time + recommendation_time + total_compression_time

    return {
        "preprocess_time": int(preprocess_time),  # Convert to integer (nanoseconds)
        "predict_time": int(predict_time),        # Convert to integer (nanoseconds)
        "recommendation_time": int(recommendation_time),  # Convert to integer (nanoseconds)
        "total_compression_time": int(total_compression_time),  # Convert to integer (nanoseconds)
        "total_time": int(total_time)             # Convert to integer (nanoseconds)
    }