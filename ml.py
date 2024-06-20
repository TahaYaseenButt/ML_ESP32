import pandas as pd
from sklearn.tree import DecisionTreeRegressor
from sklearn.model_selection import cross_val_predict, KFold
from sklearn.metrics import mean_squared_error
import numpy as np
import firebase_admin
from firebase_admin import credentials, db

def CoreTemp(training_data):
    """
    Trains a regression model and returns the trained model and validation RMSE.

    Parameters:
    training_data (pd.DataFrame): A DataFrame containing predictor and response columns.

    Returns:
    dict: A dictionary containing the trained model and other relevant information.
    float: The validation RMSE.
    """
    # Extract predictors and response
    predictor_names = ['skinTemperature', 'ambientTemperature', 'heartRate', 'weight', 'age', 'height', 'pregnancyCondition', 'gender']
    predictors = training_data[predictor_names]
    response = training_data['coreTemperature']
    
    # Train a regression model
    regression_tree = DecisionTreeRegressor(min_samples_leaf=12)
    regression_tree.fit(predictors, response)
    
    # Perform cross-validation
    kf = KFold(n_splits=5, shuffle=True, random_state=1)
    validation_predictions = cross_val_predict(regression_tree, predictors, response, cv=kf)
    
    # Compute validation RMSE
    validation_rmse = np.sqrt(mean_squared_error(response, validation_predictions))

    # Create the result dict with predict function
    trained_model = {
        'predictor_names': predictor_names,
        'regression_tree': regression_tree,
        'predictFcn': lambda x: regression_tree.predict(x[predictor_names])
    }
    
    return trained_model, validation_rmse

def on_data_change(event):
    """
    Callback function to handle data change in Firebase.
    """
    try:
        if event.data:
            # Extract and convert sensor values
            sensor_values = [float(value) for value in event.data.split(',')]
            
            # Create a DataFrame for new data
            new_data = pd.DataFrame({
                'skinTemperature': [sensor_values[0]],
                'ambientTemperature': [sensor_values[1]],
                'heartRate': [sensor_values[2]],
                'weight': [sensor_values[3]],
                'age': [sensor_values[4]],
                'height': [sensor_values[5]],
                'pregnancyCondition': [sensor_values[6]],
                'gender': [sensor_values[7]]
            })
            
            # Make predictions using the new data
            predicted_core_temperature = trained_model['predictFcn'](new_data)
            
            # Display the prediction
            print('Predicted Core Temperature:')
            print(predicted_core_temperature)
            
            # Store the prediction result back to Firebase
            result_ref = db.reference('/Result')
            result_ref.set({'predicted_core_temperature': predicted_core_temperature.tolist()})
        else:
            print("No data received in event.")
            
    except Exception as e:
        print(f'An error occurred: {e}')

if __name__ == "__main__":
    try:
        # Initialize Firebase
        cred = credentials.Certificate("datasample-5e3cd-firebase-adminsdk-mvgok-d46c4708e2.json")
        firebase_admin.initialize_app(cred, {'databaseURL': "https://datasample-5e3cd-default-rtdb.firebaseio.com"})
        
        # Specify the path to your Excel file
        excel_file_path = 'health_data.xlsx'
        
        # Read the Excel file into a pandas DataFrame
        training_data = pd.read_excel(excel_file_path)
        
        # Train the model using the existing training data
        trained_model, validation_rmse = CoreTemp(training_data)
        
        # Display the validation RMSE
        print(f'Validation RMSE: {validation_rmse}')
        
        # Set up a listener for changes in the Firebase Realtime Database
        ref = db.reference('/Data')
        ref.listen(on_data_change)
        
    except Exception as e:
        print(f'An error occurred: {e}')
