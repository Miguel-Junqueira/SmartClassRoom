import random
from paho.mqtt import client as mqtt_client
import threading
import json
import string
import datetime
import os 
import sys
import time
import mysql.connector

smartClassRoomDB = mysql.connector.connect(
  host="localhost",
  user="root",
  password="",
  database=""
)



broker = '127.0.0.1'
port = 1883
attendanceRequestTopic = "attendanceRequestServer"
attendanceResponseTopic = "attendanceResponse"
storeTempfromNodesTopic = "storeTempfromNodes"
client_id = "MainServer"
username = ''
password = ''
flag_lostCon = 0

def on_disconnect(client, userdata, rc):
   global flag_lostCon
   flag_lostCon = 1
   print("Lost Connection to MQTT Broker!")

def extract_mqtt_fields(message,topic):
    try:
        # Parse the JSON message into a Python dictionary
        message_dict = json.loads(message)
        
        # Extract individual fields from the dictionary

        if(topic == attendanceRequestTopic):

            node_id = message_dict.get("nodeID")
            local_time_date = message_dict.get("localTimeDate")
            event_type = message_dict.get("typeOfEvent")
            card_uid = message_dict.get("cardUID")
            direction = message_dict.get("direction")
            answer = message_dict.get("answer")
            student_name = message_dict.get("studentName")

            return {     
                "node_id": node_id,
                "local_time_date": local_time_date,
                "event_type": event_type,
                "card_uid": card_uid,
                "direction": direction,
                "answer": answer,
                "student_name": student_name
            }

        if(topic == storeTempfromNodesTopic):
            

            node_id = message_dict.get("nodeID")
            local_time_date = message_dict.get("localTimeDate")
            event_type = message_dict.get("typeOfEvent")
            direction = message_dict.get("direction")
            temperature = message_dict.get("temperature")
            humidity = message_dict.get("humidity")

            return {     
                "node_id": node_id,
                "local_time_date": local_time_date,
                "event_type": event_type,
                "direction": direction,
                "temperature": temperature,
                "humidity": humidity
            }


    except json.JSONDecodeError:
        print("Error: Invalid JSON message")
        return None

def connect_mqtt() -> mqtt_client:
    
    def on_connect(client, userdata, flags, rc):

        if rc == 0:
            print("Connected to MQTT Broker!\n")
            
        else:
            print("Failed to connect, return code", rc)

        if flag_lostCon == 1 and rc == 0:
            client.subscribe(attendanceRequestTopic)
            client.subscribe(attendanceResponseTopic)
            client.subscribe(storeTempfromNodesTopic)
            client.on_message = on_message  # Set the on_message callback after successful connection

    client = mqtt_client.Client(client_id)
    client.username_pw_set(username, password)
    client.on_connect = on_connect
    client.on_disconnect = on_disconnect
    client.connect(broker, port)
    return client

def on_message(client, userdata, msg):
    fields = extract_mqtt_fields(msg.payload.decode(),msg.topic)
    save_to_file = save_data_to_json_file(fields,'receivedMessages')
    if(msg.topic == attendanceRequestTopic):
        analyzeMessage(fields,client)
    if(msg.topic == storeTempfromNodesTopic):
        storeTempfromNodes(fields)

def subscribe(client: mqtt_client):
    client.subscribe(attendanceRequestTopic)
    client.subscribe(attendanceResponseTopic)
    client.subscribe(storeTempfromNodesTopic)
    client.on_message = on_message

def save_data_to_json_file(data, filename):
    try:
        with open(filename, 'a') as file:
            # Separate the new data from the existing JSON
            file.write(',')
            # Write the new JSON data
            json.dump(data, file, indent=4)
            file.write('\n')
        #print(f"Data saved to {filename} successfully.")
    except Exception as e:
        print(f"Error occurred while saving data to {filename}: {e}")

def storeTempfromNodes(fields):

    node_id = fields['node_id']
    local_time_date= fields['local_time_date']
    temp = fields['temperature']
    hum = fields['humidity']

    smartClassRoomCursor = smartClassRoomDB.cursor()
    query = "INSERT INTO tempHumidity (room, readingTimeDate, temperature, humidity) VALUES ('{}', '{}', '{}', '{}')".format(node_id, local_time_date, temp,hum)   
    smartClassRoomCursor.execute(query)
    smartClassRoomDB.commit()



def checkifAllowed(studentUID, room,client):

    found = False
    smartClassRoomCursor = smartClassRoomDB.cursor()

    query = "SELECT * FROM students WHERE studentUID = \'{}'".format(studentUID)
    smartClassRoomCursor.execute(query)
    result = smartClassRoomCursor.fetchone()

     #1 check verifica se cartao existe na base de dados

    if result:
        studentIndex = result[0]
        studentName = result[1]
        print("Found student associated with card UID: ({}) and with student index: {}".format(studentUID, result[0]))
    else:
        print("Card not found in database!")
        sendMessagetoMqtt(room,convert_to_custom_format(datetime.datetime.now()),1,studentUID,-4,"null",client)
        return
    
    #2 verificar se existem aulas naquela sala aquela hora e dia
    query = "SELECT * FROM classes WHERE room = \'{}'".format(room)
    smartClassRoomCursor.execute(query)
    results = smartClassRoomCursor.fetchall()

    for result in results:
        
        classIndex = result[0]

        if(datetime.datetime.now() >= createDateTimeObject(result[2]) and datetime.datetime.now() <= createDateTimeObject(result[3])):
            print("Found class with index {} associated with room {} at current time: {}".format(classIndex,room, datetime.datetime.now() .strftime("%H:%M:%S")))
            found = True   
            break

    if (found == False):    
        print("No class was found in the database at current time: {}".format(datetime.datetime.now()))
        sendMessagetoMqtt(room,convert_to_custom_format(datetime.datetime.now()),1,studentUID,-2,studentName,client)
        return

    found = False         
         
    #3- verificar se naquela aula em especifico o aluno em questao esta associado

    query = "SELECT classIndex FROM associatedStudents WHERE studentIndex = \'{}'".format(studentIndex)
    smartClassRoomCursor.execute(query)
    result = smartClassRoomCursor.fetchone()

    if result:
        print("Student was found in this class!")

    else:
         print("Student {} with student index {} not registered in class!".format(studentName,studentIndex))
         sendMessagetoMqtt(room,convert_to_custom_format(datetime.datetime.now()),1,studentUID,-1,studentName,client)
         return 
    
    #4 verificar se o estudante ja tem a presenca registrada naquela aula

    
    query = "SELECT classIndex FROM presentStudents WHERE studentIndex = \'{}'".format(studentIndex)
    smartClassRoomCursor.execute(query)
    result = smartClassRoomCursor.fetchone()

    if result:
        print("Student attendance already registered!")
        sendMessagetoMqtt(room,convert_to_custom_format(datetime.datetime.now()),1,studentUID,-3,studentName,client)

    else:

        if(addStudentRegistration(studentIndex,classIndex) == 0):
            print("Student attendance was registered in class!")
            sendMessagetoMqtt(room,convert_to_custom_format(datetime.datetime.now()),1,studentUID,1,studentName,client)
            return
        else:
            print("Error occurred while adding student registration!")       
            return

def addStudentRegistration(studentIndex,classIndex):
   
   try: 
   
    smartClassRoomCursor = smartClassRoomDB.cursor()
    query = "INSERT INTO presentStudents (studentIndex, classIndex) VALUES ('{}', '{}')".format(studentIndex, classIndex)   
    smartClassRoomCursor.execute(query)
    smartClassRoomDB.commit()
    return 0
   
   except mysql.connector.Error as error:
    smartClassRoomDB.rollback()
    smartClassRoomCursor.close()
    return -1

def convert_to_custom_format(current_time):

    # Extract components from current_time
    hour = current_time.hour
    minute = current_time.minute
    second = current_time.second
    day = current_time.day
    month = current_time.month
    year = current_time.year

    # Convert components to strings and pad with leading zeros if needed
    hour_str = str(hour).zfill(2)
    minute_str = str(minute).zfill(2)
    second_str = str(second).zfill(2)
    day_str = str(day).zfill(2)
    month_str = str(month).zfill(2)
    year_str = str(year)

    # Combine components into the custom format
    custom_format = f"{hour_str}{minute_str}{second_str}{day_str}{month_str}{year_str}"

    return custom_format
   
def createDateTimeObject(date_time_str):


    
    # Extract year, month, day, hour, minute, and second components
    year = int(date_time_str[10:14])
    month = int(date_time_str[8:10])
    day = int(date_time_str[6:8])
    hour = int(date_time_str[0:2])
    minute = int(date_time_str[2:4])
    second = int(date_time_str[4:6])

    # Create datetime object
    date_time_obj = datetime.datetime(year, month, day, hour, minute, second)

    return date_time_obj

def sendMessagetoMqtt(nodeID,localTimeDate,typeofEvent,cardUID,answer,studentName,client):

    message_data = {
    "nodeID": nodeID,
    "localTimeDate": localTimeDate,
    "typeOfEvent": typeofEvent,
    "cardUID": cardUID,
    "direction": 'SN',
    "answer": answer,
    "studentName": studentName
    }

    message_json = json.dumps(message_data)
    client.publish(attendanceResponseTopic, message_json)

def analyzeMessage(payload,client):

    if(payload['direction'] == 'SN'):
        return

    if(payload['event_type'] == 1):
        checkifAllowed(payload['card_uid'],payload['node_id'],client)

def request_temp(client):
    while True:
        # Publish message to the topic
        client.publish("getTempfromNodes", "0")

        # Wait for 5 minute
        time.sleep(60)
        #time.sleep(300)


def main():

    client = connect_mqtt()
    subscribe(client)

    publish_thread = threading.Thread(target=request_temp, args=(client,))
    publish_thread.start()


    client.loop_forever()





 

if __name__ == '__main__':
    main()
