import random
import json
import string
import datetime
import os 
import sys
import time
import serial
import re
import mysql.connector

smartClassRoomDB = mysql.connector.connect(
  host="localhost",
  user="root",
  password="",
  database=""
)


def add_class():

    startDate = input("Enter start date (HH-MM-SS-DD-MM-YYYY): ")
    endDate = input("Enter end date (HH-MM-SS-DD-MM-YYYY): ")
    room = input("Enter tower and room ex:(T62): ")
    classAcronym = input("Enter class acronym ex:(IC): ")
    classNumber = input("Enter class number (ex:PL2): ")
    classIndex = ''.join(random.SystemRandom().choice(string.ascii_lowercase + string.digits + string.ascii_uppercase) for _ in range(6)) #6 digit random string

    data = [
        {
        "classIndex": classIndex,
        "classNumber": classNumber,
        "startDate": startDate,
        "endDate": endDate,
        "room": room,
        "classAcronym": classAcronym
        },
    ]

    smartClassRoomCursor = smartClassRoomDB.cursor()
    query = """
    INSERT INTO classes (classIndex, classNumber, startDate, endDate, room, classAcronym)
    VALUES (%(classIndex)s, %(classNumber)s, %(startDate)s, %(endDate)s, %(room)s, %(classAcronym)s)
    """

    for row in data:
        smartClassRoomCursor.execute(query, row)

    smartClassRoomDB.commit()

    return

def add_student():

    studentName = input("Enter student full name: ")
    studentUID = input("Enter student card UID: ")
    studentIndex = ''.join(random.SystemRandom().choice(string.ascii_lowercase + string.digits + string.ascii_uppercase) for _ in range(5)) #6 digit random string

    data = [
        {
        "studentIndex": studentIndex,
        "studentName": studentName,
        "studentUID": studentUID
        },
    ]

    smartClassRoomCursor = smartClassRoomDB.cursor()
    query = """
    INSERT INTO students (studentIndex, studentName, studentUID)
    VALUES (%(studentIndex)s, %(studentName)s, %(studentUID)s)
    """

    for row in data:
        smartClassRoomCursor.execute(query, row)

    smartClassRoomDB.commit()

    return

def rem_class(classIndex):

    smartClassRoomCursor = smartClassRoomDB.cursor()

    query = "DELETE FROM associatedStudents WHERE classIndex = \'{}'".format(classIndex)
    smartClassRoomCursor.execute(query)
    smartClassRoomDB.commit()

    query = "DELETE FROM presentStudents WHERE classIndex = \'{}'".format(classIndex)
    smartClassRoomCursor.execute(query)
    smartClassRoomDB.commit()

    query = "DELETE FROM classes WHERE classIndex = \'{}'".format(classIndex)
    smartClassRoomCursor.execute(query)
    smartClassRoomDB.commit()

def rem_student(studentIndex):

    smartClassRoomCursor = smartClassRoomDB.cursor()

    query = "DELETE FROM associatedStudents WHERE studentIndex = \'{}'".format(studentIndex)
    smartClassRoomCursor.execute(query)
    smartClassRoomDB.commit()

    query = "DELETE FROM presentStudents WHERE studentIndex = \'{}'".format(studentIndex)
    smartClassRoomCursor.execute(query)
    smartClassRoomDB.commit()

    query = "DELETE FROM students WHERE studentIndex = \'{}'".format(studentIndex)
    smartClassRoomCursor.execute(query)
    smartClassRoomDB.commit()



def add_stundent_to_class(classIndex,studentIndex):

    try:
        smartClassRoomCursor = smartClassRoomDB.cursor()
        query= "INSERT INTO associatedStudents (studentIndex, classIndex) VALUES ('{}', '{}')".format(studentIndex, classIndex)
        smartClassRoomCursor.execute(query)
        smartClassRoomDB.commit()
        return 0
   
    except mysql.connector.Error as error:
        smartClassRoomDB.rollback()
        smartClassRoomCursor.close()
        return -1

def add_student_via_reading():


    print("")

def manage_students_menu():

    clear_screen()

    while True:
        print("Manage Students Menu:")
        print("1. Add students manually to database")
        print("2. Add student via card UID reading")
        print("3. Remove students from database")
        print("4. Change students associated card UID")
        print("5. View students in database")
        print("6. Go back to main menu")

        choice = input("Enter your choice: ")

        if choice == '1':
            clear_screen()
            add_student()
            print("Added student data successfully!")
            time.sleep(3)
            clear_screen()

        elif choice == '2':
            print("Not Implemented!")
            time.sleep(1)
            clear_screen()

        elif choice == '3':
            clear_screen()
            studentIndex = viewStudentsInDB(True)
            rem_student(studentIndex)
            time.sleep(1)
            clear_screen()

        elif choice == '4':
            print("Not Implemented!")
            time.sleep(1)
            clear_screen()

        elif choice == '5':
            clear_screen()
            viewStudentsInDB(False)
            choice = input("Enter any key to go back: ")
            clear_screen()

        elif choice == '6':
            clear_screen()
            return
        
        else:
            print("Invalid choice. Please enter a valid option.")
            time.sleep(0.5)
            clear_screen()

def viewCurrentClasses(enableSelection):

    smartClassRoomCursor = smartClassRoomDB.cursor()
    query = "SELECT * FROM classes"
    smartClassRoomCursor.execute(query)
    results = smartClassRoomCursor.fetchall()


    print("Classes:")
    index = 1

    for result in results:
        print(f"{index}. {result[5]} - {result[1]} - {result[0]} - {result[4]} - {'Starts at: '} {createDateTimeObject(result[2])} - {'Ends at: '} {createDateTimeObject(result[3])}")
        index = index + 1

    if enableSelection == True:
        choice = int(input("Please select the desired class: "))
        return results[choice-1][0]

    #adicionar salvaguardas para opcoes erradas

def clear_screen():
    # Clear the console screen
    os.system('cls' if os.name == 'nt' else 'clear')

def manage_classes_menu():

    clear_screen()
    enableSelection = False

    while True:

        print("Manage Classes Menu:")
        print("1. Add classes to schedule")
        print("2. Remove classes from schedule")
        print("3. Add students to class")
        print("4. View current classes")
        print("5. Go back to main menu")

        choice = input("Enter your choice: ")

        if choice == '1':
            clear_screen()
            add_class()
            print("Class data added successfully!")
            time.sleep(2)
            clear_screen()

        elif choice == '2':
            clear_screen()
            classIndex = viewCurrentClasses(True)
            rem_class(classIndex)
            time.sleep(1)
            clear_screen()

        elif choice == '3':
            clear_screen()
            addStudentstoClassMenu1()

        elif choice == '4':
            clear_screen()
            viewCurrentClasses(False)
            choice = input("Enter any key to go back: ")
            clear_screen()

        elif choice == '5':
            clear_screen()
            return

        else:
            print("Invalid choice. Please enter a valid option.")
            time.sleep(0.5)
            clear_screen()

def addStudentstoClassMenu1():

    clear_screen()

    while True:

        print("How do you wish to select desired class?")
        print("1. Select from classes database")
        print("2. Search for class by date")
        print("3. Search for class by acronym")
        print("3. Search for class by room number")
        print("4. Manually enter class index")
        print("5. Return to previous menu")

        choice = input("Enter your choice: ")

        if choice == '1':
            clear_screen()
            classIndex = viewCurrentClasses(True)
            clear_screen()
            addStudentstoClassMenu2(classIndex)
            return

        elif choice == '2':
            print("Not Implemented!")
            time.sleep(1)
            clear_screen()

        elif choice == '3':
            print("Not Implemented!")
            time.sleep(1)
            clear_screen()

        elif choice == '4':
            print("Not Implemented!")
            time.sleep(1)
            clear_screen()

        elif choice == '5':
            clear_screen()
            return

def addStudentstoClassMenu2(classIndex):

        while True:

            print("How do you wish to add students to the selected class?")
            print("1. Select from students database")
            print("2. Search for student by name")
            print("3. Search for student by card UID")
            print("3. Search for student by index")
            print("4. Manually enter student index")
            print("5. Return to previous menu")

            choice = input("Enter your choice: ")

            if choice == '1':
                clear_screen()
                studentIndex = viewStudentsInDB(True)
                if (add_stundent_to_class(classIndex,studentIndex) == 0):
                    print("Added student to class successfully!")
                else:
                    print("Student is already enrolled in this class!")
                time.sleep(2)
                clear_screen()
                return

            elif choice == '2':
                print("Not Implemented!")
                time.sleep(1)
                clear_screen()

            elif choice == '3':
                print("Not Implemented!")
                time.sleep(1)
                clear_screen()

            elif choice == '4':
                print("Not Implemented!")
                time.sleep(1)
                clear_screen()

            elif choice == '5':
                clear_screen()
                return

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

def viewStudentsInDB(enableSelection):

    smartClassRoomCursor = smartClassRoomDB.cursor()
    query = "SELECT * FROM students"
    smartClassRoomCursor.execute(query)
    results = smartClassRoomCursor.fetchall()

    print("Current students in database:")
    index = 1

    #for index, class_data in enumerate(students_data, start=1):
     #   print(f"{index}. {class_data['studentIndex']} - {class_data['studentName']} - {class_data['studentUID']} ")

    for result in results:
        print(f"{index}. {result[0]} - {result[1]} - {result[2]}")
        index = index + 1

    if enableSelection == True:
        choice = int(input("Please select the desired student: "))
        return results[choice-1][0]
    #adicionar salvaguardas para opcoes erradas
    
def manage_iot_nodes_menu():
    print("\nManage IoT Nodes Menu:")
    print("1. View connected IoT Nodes")

def smart_classroom_admin_menu():
    while True:
        print("Smart Classroom ADMIN Menu:")
        print("1. Manage Students")
        print("2. Manage Classes")
        print("3. Manage IoT Nodes")
        print("0. Exit")

        choice = input("Enter your choice: ")

        if choice == '1':
           manage_students_menu()  
        elif choice == '2':
            manage_classes_menu()
        elif choice == '3':
            print("Not Implemented!")
            time.sleep(1)
            clear_screen()
        elif choice == '0':
            print("Exiting smartClassRoom menu!")
            return
        
        else:
            print("Invalid choice. Please enter a valid option.")
            time.sleep(0.5)
            clear_screen()

def main():

    smart_classroom_admin_menu()


if __name__ == '__main__':
    main()
