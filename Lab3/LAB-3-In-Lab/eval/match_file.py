def print_passed(msg):
    print("\033[92m" + msg + "\033[0m")


def print_failed(msg):
    print("\033[91m" + msg + "\033[0m")


def match_file():
    f1 = open("f1.txt", "r")
    student = open("client_file.txt", "r")
    if f1.read() == student.read():
        print_passed("test_client_file - TEST CASE PASSED")
        return 3
    else:
        print_failed("test_client_file - TEST CASE FAILED")
        return 0

if __name__ == "__main__":
    marks = 0
    marks += match_file()
    print("Marks: ", marks)