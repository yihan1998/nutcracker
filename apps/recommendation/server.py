from flask import Flask, request, jsonify
import pickle

app = Flask(__name__)

# Load the pre-trained recommendation model
with open('/home/ubuntu/Nutcracker/apps/recommendation/recommendation_model.pkl', 'rb') as model_file:
    algo = pickle.load(model_file)

@app.route('/recommend', methods=['GET'])
def recommend():
    # Get user ID from the request
    user_id = request.args.get('user_id')
    if not user_id:
        return jsonify({'error': 'User ID is required'}), 400

    # Generate recommendations for the user
    try:
        user_id = int(user_id)
        # Here we assume that movie IDs range from 1 to 1682 (as per MovieLens 100k dataset)
        recommendations = []
        for movie_id in range(1, 1683):
            pred = algo.predict(user_id, movie_id)
            recommendations.append((movie_id, pred.est))
        
        # Sort recommendations by estimated rating
        recommendations.sort(key=lambda x: x[1], reverse=True)
        
        # Return top 10 recommendations
        top_recommendations = recommendations[:10]
        return jsonify(top_recommendations)
    except ValueError:
        return jsonify({'error': 'Invalid User ID'}), 400

def entrypoint():
    print("Hello world!\n")
    app.run(host='0.0.0.0', port=5000)
