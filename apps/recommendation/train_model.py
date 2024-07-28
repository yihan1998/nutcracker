from surprise import Dataset, Reader, SVD
from surprise.model_selection import train_test_split
import pickle

# Load the movielens-100k dataset (download it if needed)
data = Dataset.load_builtin('ml-100k')

# Train-test split
trainset, testset = train_test_split(data, test_size=0.25)

# Use the famous SVD algorithm
algo = SVD()

# Train the algorithm on the trainset
algo.fit(trainset)

# Save the trained model to a file
with open('recommendation_model.pkl', 'wb') as model_file:
    pickle.dump(algo, model_file)
