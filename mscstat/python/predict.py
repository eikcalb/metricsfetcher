import sys
import json
import tensorflow as tf

def load_model(model_path):
    model = tf.keras.models.load_model(model_path)
    return model

def make_predictions(model, input_dict):
    predictions = model.predict(input_dict)
    return predictions

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: python predict.py input_json_string")
        sys.exit(1)

    input_dict = json.loads(sys.argv[1])

    model = load_model(input_dict['modelPath'])

    # Make predictions using the model and input data
    predictions = make_predictions(model, input_dict)

    print(predictions)
