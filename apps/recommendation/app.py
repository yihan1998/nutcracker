from flask import Flask, request, jsonify
import tensorflow as tf

app = Flask(__name__)

# Load the trained model
model = tf.keras.models.load_model("movielens_model")

@app.route('/predict', methods=['POST'])
def predict():
    data = request.get_json()
    user_id = data['user_id']
    movie_title = data['movie_title']

    user_embedding = model.user_model(tf.constant([user_id]))
    movie_embedding = model.movie_model(tf.constant([movie_title]))

    score = tf.tensordot(user_embedding, movie_embedding, axes=2)[0][0].numpy()

    return jsonify({
        'user_id': user_id,
        'movie_title': movie_title,
        'predicted_score': score
    })

if __name__ == '__main__':
    app.run(debug=True, host='0.0.0.0', port=5000)
