import sys
import json
import sqlite3
import pandas as pd
import numpy as np
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
    cpuMetrics['timestamp'] = (cpuMetrics['timestamp'] // 100) * 100
    cpuMetrics['counterU'] = cpuMetrics['timestamp'].astype(str) + cpuMetrics['counter'].astype(str)
    cpuMetrics['counterU'] = cpuMetrics['counterU'].astype("int64")

    processMetrics.drop(columns="name", inplace=True)
    processMetrics['timestamp'] = (processMetrics['timestamp'] // 100) * 100
    processMetrics['counterU'] = processMetrics['timestamp'].astype(str) + processMetrics['counter'].astype(str)
    processMetrics['counterU'] = processMetrics['counterU'].astype("int64")

    memoryMetrics.drop(columns="name", inplace=True)
    memoryMetrics['timestamp'] = (memoryMetrics['timestamp'] // 100) * 100
    memoryMetrics['counterU'] = memoryMetrics['timestamp'].astype(str) + memoryMetrics['counter'].astype(str)
    memoryMetrics['counterU'] = memoryMetrics['counterU'].astype("int64")

    storageMetrics.drop(columns="name", inplace=True)
    storageMetrics['timestamp'] = (storageMetrics['timestamp'] // 100) * 100
    storageMetrics['counterU'] = storageMetrics['timestamp'].astype(str) + storageMetrics['counter'].astype(str)
    storageMetrics['counterU'] = storageMetrics['counterU'].astype('Int64')

    scriptMetrics['timestamp'] = (scriptMetrics['timestamp'] // 100) * 100
    scriptMetrics['counterU'] = scriptMetrics['timestamp'].astype(str) + scriptMetrics['counter'].astype(str)
    scriptMetrics['counterU'] = scriptMetrics['counterU'].astype('Int64')
    scriptMetrics = scriptMetrics.pivot(index='counterU', columns='key', values='value').reset_index()

    # cpuMetrics["t_cpu_label"] = 0
    # processMetrics["t_proc_label"] = 0;
    # memoryMetrics["t_mem_label"] = 0;
    # storageMetrics["t_store_label"] = 0;
    # scriptMetrics["t_script_label"] = 0;

    # Merge the tables
    cpuMetrics.drop(columns=["counter", "id", "timestamp"], inplace=True)
    processMetrics.drop(columns=["counter", "id", "timestamp"], inplace=True)
    memoryMetrics.drop(columns=["counter", "id", "timestamp"], inplace=True)
    storageMetrics.drop(columns=[ "counter", "id", "timestamp"], inplace=True)

    combined_data = pd.merge(cpuMetrics, processMetrics, on='counterU', how='inner')
    combined_data = pd.merge(combined_data, memoryMetrics, on='counterU', how='inner')
    combined_data = pd.merge(combined_data, storageMetrics, on='counterU', how='inner')
    combined_data = pd.merge(combined_data, scriptMetrics, on='counterU', how='inner')
    combined_data = combined_data.dropna()

    colummns_to_encode = ['activeProcess', 'activeWindow']
    combined_data = pd.get_dummies(combined_data, columns=colummns_to_encode, prefix=colummns_to_encode, drop_first=True)
    combined_data.fillna(0, inplace=True)
    combined_data.drop(['counterU'], axis=1, inplace=True)

    num_timesteps = 1

    X = combined_data
    X = X.values.reshape((X.shape[0], num_timesteps, X.shape[1]))

    X = tf.constant(X, dtype=tf.float32)

    predictions = model.predict(X)
    return predictions

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: python predict.py input_json_string")
        sys.exit(1)

    input_dict = json.loads(sys.argv[1])

    db_path = input_dict["databasePath"]
    conn = sqlite3.connect(db_path)
    model = load_model(input_dict['modelPath'])

    try:
        # Make predictions using the model and input data
        predictions = make_predictions(model, conn)
        conn.close()

        print(predictions)
    except:
        conn.close()
        sys.exit(1);
