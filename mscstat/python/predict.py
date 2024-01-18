import sys
import sqlite3
import pandas as pd
from sklearn.preprocessing import LabelEncoder
import tensorflow as tf
import tensorflow as tf

def load_model(model_path):
    model = tf.keras.models.load_model(model_path)
    return model

def make_predictions(model, conn):
    cpuMetrics = pd.read_sql_query("SELECT * FROM CPUMetricProvider WHERE 1=1 ORDER BY id DESC LIMIT 1", conn)
    processMetrics = pd.read_sql_query("SELECT * FROM ProcessMetricProvider WHERE 1=1 ORDER BY id DESC LIMIT 1", conn)
    memoryMetrics = pd.read_sql_query("SELECT * FROM RAMMetricProvider WHERE 1=1 ORDER BY id DESC LIMIT 1", conn)
    storageMetrics = pd.read_sql_query("SELECT * FROM StorageMetricProvider WHERE 1=1 ORDER BY id DESC LIMIT 1", conn)
    scriptMetrics = pd.read_sql_query("SELECT * FROM ScriptData WHERE 1=1 AND 'key'=\"ProcessorPerformance\" ORDER BY id DESC LIMIT 1", conn)
    # endregion

    cpuMetrics.drop(columns="name", inplace=True)
    cpuMetrics['timestamp'] = (cpuMetrics['timestamp'] // 1000) * 1000
    cpuMetrics['counterU'] = cpuMetrics['timestamp'].astype(str) + cpuMetrics['counter'].astype(str)
    cpuMetrics['counterU'] = cpuMetrics['counterU'].astype("int64")

    processMetrics.drop(columns="name", inplace=True)
    processMetrics['timestamp'] = (processMetrics['timestamp'] // 1000) * 1000
    processMetrics['counterU'] = processMetrics['timestamp'].astype(str) + processMetrics['counter'].astype(str)
    processMetrics['counterU'] = processMetrics['counterU'].astype("int64")

    memoryMetrics.drop(columns="name", inplace=True)
    memoryMetrics['timestamp'] = (memoryMetrics['timestamp'] // 1000) * 1000
    memoryMetrics['counterU'] = memoryMetrics['timestamp'].astype(str) + memoryMetrics['counter'].astype(str)
    memoryMetrics['counterU'] = memoryMetrics['counterU'].astype("int64")

    storageMetrics.drop(columns="name", inplace=True)
    storageMetrics['timestamp'] = (storageMetrics['timestamp'] // 1000) * 1000
    storageMetrics['counterU'] = storageMetrics['timestamp'].astype(str) + storageMetrics['counter'].astype(str)
    storageMetrics['counterU'] = storageMetrics['counterU'].astype('Int64')

    scriptMetrics['timestamp'] = (scriptMetrics['timestamp'] // 1000) * 1000
    scriptMetrics['counterU'] = scriptMetrics['timestamp'].astype(str) + scriptMetrics['counter'].astype(str)
    scriptMetrics['counterU'] = scriptMetrics['counterU'].astype('Int64')
    scriptMetrics = scriptMetrics.pivot(index='counterU', columns='key', values='value').reset_index()

    # This will account for situatuions where the script for processor performance has not been setup
    if "ProcessorPerformance" not in scriptMetrics.columns:
        scriptMetrics["ProcessorPerformance"] = 0

    # Merge the tables
    cpuMetrics.drop(columns=["counter", "id", "timestamp"], inplace=True)
    processMetrics.drop(columns=["counter", "id", "timestamp"], inplace=True)
    memoryMetrics.drop(columns=["counter", "id", "timestamp"], inplace=True)
    storageMetrics.drop(columns=[ "counter", "id", "timestamp"], inplace=True)

    combined_data = pd.merge(cpuMetrics, processMetrics, on='counterU', how='left')
    combined_data = pd.merge(combined_data, memoryMetrics, on='counterU', how='left')
    combined_data = pd.merge(combined_data, storageMetrics, on='counterU', how='left')
    combined_data = pd.merge(combined_data, scriptMetrics, on='counterU', how='left')
    # combined_data = combined_data.dropna()
    print([col for col in combined_data.columns])
    columns_to_encode = ['activeProcess', 'activeWindow']
    for column in columns_to_encode:
        le = LabelEncoder()
        combined_data[column] = le.fit_transform(combined_data[column])

    combined_data.fillna(0, inplace=True)
    combined_data.drop(['counterU'], axis=1, inplace=True)

    X = combined_data
    X = X.values.reshape((1, X.shape[0], X.shape[1]))

    # X = tf.constant(X, dtype=tf.float32)

    threshold = 0.75
    predictions = model.predict(X)
    predictions = [item[0] for item in predictions]
    predictions =  [1 if val > threshold else 0 for val in predictions]
    return predictions

def main():
    print(sys.argv[1]);
    if len(sys.argv) < 3 or len(sys.argv) > 4:
        print("Usage: python predict.py <modelPath> <databasePath> [outputPath]")
        sys.exit(1)


    model_path = sys.argv[1]
    db_path = sys.argv[2]
    output_path = sys.argv[3] if len(sys.argv) > 3 else "output.pred"
    conn = sqlite3.connect(db_path)
    model = load_model(model_path)
    # pdb.set_trace();

    try:
        # Make predictions using the model and input data
        predictions = make_predictions(model, conn)
        conn.close()

        with open(output_path, 'w') as file:
            file.write(",".join(map(str, predictions)))
    except Exception as ex:
        print(ex);
        conn.close()
        sys.exit(1);

if __name__ == "__main__":
    main()