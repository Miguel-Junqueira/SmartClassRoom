import random
import json
import string
import datetime
import os 
import sys
import time

def add_class():

    start_date = input("Enter start date (HH-MM-SS-DD-MM-YYYY): ")
    end_date = input("Enter end date (HH-MM-SS-DD-MM-YYYY): ")
    room = input("Enter tower and room ex:(T62): ")
    class_acronym = input("Enter class acronym ex:(IC): ")
    class_number = input("Enter class number (ex:PL2): ")
    classIndex = ''.join(random.SystemRandom().choice(string.ascii_lowercase + string.digits + string.ascii_uppercase) for _ in range(6)) #6 digit random string
    #start_date = "18000028032024"
    #end_date = "20000028032024"
    #room = "T51"
    #class_acronym = "IC"
    #class_number = "PL1"


    class_data = {
        "classIndex": classIndex,
        "classNumber": class_number,
        "startDate": start_date,
        "endDate": end_date,
        "room": room,
        "associatedStudentsIndex": [''],
        "presentStudents": [''],
        "class_acronym": class_acronym
    }


    #class_json = json.dumps(class_data, indent=4)
    filename = "classesDB.json"

    
    try:
        # Serialize the class_data dictionary to JSON format
        class_json = json.dumps(class_data, indent=4)

        # Check if the file already exists
        file_exists = True
        try:
            with open(filename, 'r') as file:
                file_content = file.read().strip()
                if not file_content:
                    file_exists = False
                elif file_content.endswith(']'):
                    # Strip the closing bracket from the existing content
                    file_content = file_content[:-1]
                    with open(filename, 'w') as file:
                        file.write(file_content)

        except FileNotFoundError:
            file_exists = False

        # Add delimiters and commas if the file already exists
        if file_exists:
            class_json = ",\n" + class_json
        else:
            class_json = "[\n" + class_json

        # Append the class_json string to the file
        with open(filename, 'a') as file:
            file.write(class_json)

        # Close the array if the file already exists
        if file_exists:
            with open(filename, 'a') as file:
                file.write("\n]")

        if not(file_exists):
            with open(filename, 'a') as file:
                file.write("\n]") #apend the last ]


        print(f"Class data saved to {filename} successfully.")
    except Exception as e:
        print(f"Error occurred while saving class data to {filename}: {e}")

def add_student():

    studentName = input("Enter student full name: ")
    studentUID = input("Enter student card UID: ")
    studentIndex = ''.join(random.SystemRandom().choice(string.ascii_lowercase + string.digits + string.ascii_uppercase) for _ in range(5)) #6 digit random string

    student_data = {
        "studentIndex": studentIndex,
        "studentName": studentName,
        "studentUID": studentUID
    }

    #student_json = json.dumps(student_data, indent=4)

    filename = f"studentsDB.json"


    try:
        student_json = json.dumps(student_data, indent=4)

        # Check if the file already exists
        file_exists = True
        try:
            with open(filename, 'r') as file:
                file_content = file.read().strip()
                if not file_content:
                    file_exists = False
                elif file_content.endswith(']'):
                    # Strip the closing bracket from the existing content
                    file_content = file_content[:-1]
                    with open(filename, 'w') as file:
                        file.write(file_content)

        except FileNotFoundError:
            file_exists = False

        # Add delimiters and commas if the file already exists
        if file_exists:
            student_json = ",\n" + student_json
        else:
            student_json = "[\n" + student_json

        # Append the class_json string to the file
        with open(filename, 'a') as file:
            file.write(student_json)

        # Close the array if the file already exists
        if file_exists:
            with open(filename, 'a') as file:
                file.write("\n]")

        if not(file_exists):
            with open(filename, 'a') as file:
                file.write("\n]") #apend the last ]


        print(f"Student data saved to {filename} successfully.")
    except Exception as e:
        print(f"Error occurred while saving student data to {filename}: {e}")







  #  try:
   #     with open(filename, 'a') as file:
    ##        file.write(student_json + '\n')
      #  print(f"Student data saved to {filename} successfully.")
    #except Exception as e:
     #   print(f"Error occurred while saving student data to {filename}: {e}")

def add_stundent_to_class(classIndex,studentIndex):



    with open('classesDB.json', 'r') as file:
        classes_data = json.load(file)


    # Iterate through each entry in the dictionary
        
    for entry in classes_data:
        for key, entry_string in entry.items():

            # Check if the value of the "classIndex" key matches the desired value
            if entry_string == classIndex:
                
                for key, entry_string in entry.items():

                    if(key == "associatedStudentsIndex"):

                        if entry[key] == ['']:
                            entry[key] = [studentIndex] 
                        else:                            
                           entry[key].append(studentIndex)
                        break
            break
                
        
        else:
            print("Desired class index not found in any entry")

    





    # Write the updated JSON data back to the file
    with open('classesDB.json', 'w') as file:
        json.dump(classes_data, file, indent=4)

    print("Student {} successfully added to class {}".format(studentIndex,classIndex))

def manage_students_menu():

    clear_screen()

    while True:
        print("Manage Students Menu:")
        print("1. Add students to database")
        print("2. Remove students from database")
        print("3. Change students associated card UID")
        print("4. View students in databasse")
        print("5. Go back to main menu")

        choice = input("Enter your choice: ")

        if choice == '1':
            clear_screen()
            add_student()
            time.sleep(3)
            clear_screen()

        elif choice == '2':
            print("Not Implemented!")
            time.sleep(1)
            clear_screen()

        elif choice == '3':
            print("Not Implemented!")
            time.sleep(1)
            clear_screen()

        elif choice == '4':
            clear_screen()
            viewStudentsInDB(False)
            choice = input("Enter any key to go back: ")
            clear_screen()

        elif choice == '5':
            clear_screen()
            return
        
        else:
            print("Invalid choice. Please enter a valid option.")
            time.sleep(0.5)
            clear_screen()

def viewCurrentClasses(enableSelection):

    with open('classesDB.json', 'r') as file:
        classes_data = json.load(file)

    print("Classes:")

    for index, class_data in enumerate(classes_data, start=1):
        print(f"{index}. {class_data['class_acronym']} - {class_data['classNumber']} - {class_data['classIndex']} - {class_data['room']} - {'Starts at: '} {createDateTimeObject(class_data['startDate'])} - {'Ends at: '} {createDateTimeObject(class_data['endDate'])}")

    if enableSelection == True:
        choice = int(input("Please select the desired class: "))
        return classes_data[choice-1]['classIndex']

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
            time.sleep(2)
            clear_screen()

        elif choice == '2':
            print("Not Implemented!")
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
                add_stundent_to_class(classIndex,studentIndex)
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

    with open('studentsDB.json', 'r') as file:
        students_data = json.load(file)

    print("Current students in database:")

    for index, class_data in enumerate(students_data, start=1):
        print(f"{index}. {class_data['studentIndex']} - {class_data['studentName']} - {class_data['studentUID']} ")

    if enableSelection == True:
        choice = int(input("Please select the desired class: "))
        return students_data[choice-1]['studentIndex']
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