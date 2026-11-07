#include <cctype>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#ifdef _WIN32
#include <windows.h>
#endif
using namespace std;

// เก็บข้อมูลเกรดของหนึ่งรายวิชา โดยระบุได้ว่ามีคะแนนตัวเลขหรือมีเฉพาะเกรดเดิม
struct SubjectGrade {
    string subject;
    string grade;
    int score = 0;
    bool hasScore = false;
};

// เก็บข้อมูลพื้นฐานของนักเรียนและรายการเกรดของทุกวิชา
struct Student {
    string id;
    string name;
    string room;
    vector<SubjectGrade> grades;
};

// กำหนดชื่อไฟล์ที่ใช้จัดเก็บข้อมูล และข้อความอธิบายช่วงคะแนนของแต่ละเกรด
const string FILE_NAME = "students.txt";
const string GRADE_SCALE =
    "80-100=A, 75-79=B+, 70-74=B, 65-69=C+, "
    "60-64=C, 55-59=D+, 50-54=D, 0-49=F";

// แยกข้อมูลหนึ่งบรรทัดตามเครื่องหมาย | เพื่ออ่านแต่ละฟิลด์จากไฟล์
void splitRecord(const string& line, vector<string>& fields) {
    fields.clear();
    size_t start = 0;

    while (true) {
        size_t separator = line.find('|', start);
        fields.push_back(line.substr(start, separator - start));
        if (separator == string::npos) {
            break;
        }
        start = separator + 1;
    }

}

// ตัดช่องว่างและอักขระขึ้นบรรทัดใหม่ที่หัวและท้ายข้อความ
void trim(string& value) {
    size_t first = value.find_first_not_of(" \t\r\n");
    if (first == string::npos) {
        value.clear();
        return;
    }

    size_t last = value.find_last_not_of(" \t\r\n");
    value = value.substr(first, last - first + 1);
}

// แปลงข้อความเป็นคะแนนจำนวนเต็ม และปฏิเสธค่าที่มีอักขระเกินหรืออยู่นอกช่วง 0-100
void parseScore(const string& input, int& score, bool& isValid) {
    istringstream parser(input);
    char extra;
    isValid = (parser >> score) && !(parser >> extra) &&
              score >= 0 && score <= 100;
}

// แปลงคะแนนตัวเลขเป็นเกรดตามเกณฑ์ที่กำหนด
void calculateGrade(int score, string& grade) {
    if (score >= 80) grade = "A";
    else if (score >= 75) grade = "B+";
    else if (score >= 70) grade = "B";
    else if (score >= 65) grade = "C+";
    else if (score >= 60) grade = "C";
    else if (score >= 55) grade = "D+";
    else if (score >= 50) grade = "D";
    else grade = "F";
}

// ตัดช่องว่างและแปลงตัวอักษรเกรดเป็นพิมพ์ใหญ่เพื่อให้ตรวจสอบได้สม่ำเสมอ
void normalizeGrade(string& grade) {
    trim(grade);
    for (char& character : grade) {
        character = static_cast<char>(
            toupper(static_cast<unsigned char>(character)));
    }
}

// ตรวจสอบว่าเป็นสัญลักษณ์เกรดที่ระบบรองรับหรือไม่
void isValidGrade(const string& grade, bool& isValid) {
    isValid = grade == "A" || grade == "B+" || grade == "B" ||
              grade == "C+" || grade == "C" || grade == "D+" ||
              grade == "D" || grade == "F";
}

// แปลงเกรดตัวอักษรเป็นคะแนนเกรด 4.0 เพื่อใช้คำนวณเกรดเฉลี่ย
void gradePoint(const string& grade, double& points) {
    if (grade == "A") points = 4.0;
    else if (grade == "B+") points = 3.5;
    else if (grade == "B") points = 3.0;
    else if (grade == "C+") points = 2.5;
    else if (grade == "C") points = 2.0;
    else if (grade == "D+") points = 1.5;
    else if (grade == "D") points = 1.0;
    else points = 0.0;
}

// คำนวณ GPA โดยเฉลี่ยคะแนนเกรดของแต่ละวิชาเท่ากัน
void calculateGpa(const Student& student, double& gpa, bool& hasGpa) {
    double totalPoints = 0.0;
    size_t subjectCount = 0;

    for (const SubjectGrade& grade : student.grades) {
        bool validGrade = false;
        isValidGrade(grade.grade, validGrade);
        if (validGrade) {
            double points = 0.0;
            gradePoint(grade.grade, points);
            totalPoints += points;
            ++subjectCount;
        }
    }

    if (subjectCount == 0) {
        gpa = 0.0;
        hasGpa = false;
        return;
    }

    gpa = totalPoints / subjectCount;
    hasGpa = true;
}

// ค้นหานักเรียนด้วยรหัสและคืนตำแหน่ง; คืนขนาดเวกเตอร์เมื่อไม่พบ
void findStudent(const vector<Student>& students, const string& id,
                 size_t& studentIndex) {
    for (size_t i = 0; i < students.size(); ++i) {
        if (students[i].id == id) {
            studentIndex = i;
            return;
        }
    }
    studentIndex = students.size();
}

// โหลดข้อมูลนักเรียนและรายวิชาจากไฟล์
void loadStudents(vector<Student>& students) {
    students.clear();
    ifstream file(FILE_NAME);

    if (!file.is_open()) {
        return;
    }

    string line;
    size_t lineNumber = 0;
    while (getline(file, line)) {
        ++lineNumber;
        vector<string> fields;
        splitRecord(line, fields);
        if (fields.size() < 3 || fields[0].empty() ||
            fields[1].empty() || fields[2].empty()) {
            cout << "ข้ามข้อมูลที่ไม่ถูกต้องในไฟล์ บรรทัดที่ "
                 << lineNumber << "\n";
            continue;
        }

        // รวมแถวที่มีรหัสเดียวกันไว้ในนักเรียนคนเดิม พร้อมตรวจสอบชื่อและห้องให้ตรงกัน
        size_t studentIndex = 0;
        findStudent(students, fields[0], studentIndex);
        if (studentIndex == students.size()) {
            Student student;
            student.id = fields[0];
            student.name = fields[1];
            student.room = fields[2];
            students.push_back(student);
            studentIndex = students.size() - 1;
        } else if (students[studentIndex].name != fields[1] ||
                   students[studentIndex].room != fields[2]) {
            cout << "ข้ามข้อมูลที่ไม่ตรงกับนักเรียนรหัส " << fields[0]
                 << " ในบรรทัดที่ " << lineNumber << "\n";
            continue;
        }

        // รูปแบบปัจจุบันมี 5 ฟิลด์: รหัส ชื่อ ห้อง วิชา และคะแนนหรือเกรด
        if (fields.size() == 5 && !fields[3].empty()) {
            SubjectGrade grade;
            grade.subject = fields[3];
            bool validScore = false;
            parseScore(fields[4], grade.score, validScore);
            if (validScore) {
                grade.hasScore = true;
                calculateGrade(grade.score, grade.grade);
            } else {
                string existingGrade = fields[4];
                normalizeGrade(existingGrade);
                bool validGrade = false;
                isValidGrade(existingGrade, validGrade);
                if (validGrade) {
                    grade.grade = existingGrade;
                } else {
                    cout << "ข้ามข้อมูลคะแนนที่ไม่ถูกต้องในไฟล์ บรรทัดที่ "
                         << lineNumber << "\n";
                    continue;
                }
            }
            students[studentIndex].grades.push_back(grade);
        } else if (fields.size() == 4 && !fields[3].empty()) {
            cout << "พบคะแนนรวมรูปแบบเดิมของรหัส " << fields[0]
                 << " ซึ่งไม่สามารถระบุวิชาได้ จึงยังไม่แปลงเป็นเกรดรายวิชา\n";
        } else if (fields.size() > 3 && fields.size() != 5) {
            cout << "ข้ามข้อมูลวิชาที่ไม่ถูกต้องในไฟล์ บรรทัดที่ "
                 << lineNumber << "\n";
        }
    }

}

// บันทึกข้อมูลนักเรียนและเกรดรายวิชาทั้งหมด
void saveStudents(const vector<Student>& students) {
    ofstream file(FILE_NAME);
    if (!file.is_open()) {
        cout << "ไม่สามารถเปิดไฟล์เพื่อบันทึกได้\n";
        return;
    }

    // เขียนหนึ่งแถวต่อนักเรียนหนึ่งวิชา; นักเรียนที่ยังไม่มีวิชาเขียนเฉพาะข้อมูลพื้นฐาน
    for (const Student& student : students) {
        if (student.grades.empty()) {
            file << student.id << "|" << student.name << "|"
                 << student.room << "\n";
            continue;
        }

        for (const SubjectGrade& grade : student.grades) {
            file << student.id << "|" << student.name << "|"
                 << student.room << "|" << grade.subject << "|"
                 << (grade.hasScore ? to_string(grade.score) : grade.grade)
                 << "\n";
        }
    }

    if (!file) {
        cout << "เกิดข้อผิดพลาดระหว่างบันทึกข้อมูล\n";
        return;
    }
    cout << "บันทึกข้อมูลเรียบร้อยแล้ว\n";
}

// แสดงข้อมูลพื้นฐานของนักเรียนและเกรดรายวิชาทั้งหมด
void showStudent(const Student& student) {
    cout << "\nรหัสนักเรียน : " << student.id << "\n";
    cout << "ชื่อ-นามสกุล : " << student.name << "\n";
    cout << "ห้องเรียน    : " << student.room << "\n";
    double gpa = 0.0;
    bool hasGpa = false;
    calculateGpa(student, gpa, hasGpa);
    if (hasGpa) {
        cout << "เกรดเฉลี่ย (GPA): " << fixed << setprecision(2)
             << gpa << defaultfloat << "\n";
    } else {
        cout << "เกรดเฉลี่ย (GPA): ไม่มีข้อมูล\n";
    }
    cout << "----- คะแนนและเกรดรายวิชา -----\n";

    if (student.grades.empty()) {
        cout << "ยังไม่มีข้อมูลรายวิชา\n";
        return;
    }

    for (const SubjectGrade& grade : student.grades) {
        cout << "วิชา: " << grade.subject << " | ";
        if (grade.hasScore) {
            cout << "คะแนน: " << grade.score << "/100 | เกรด: "
                 << grade.grade;
        } else {
            cout << "คะแนน: ไม่มีข้อมูล | เกรดเดิม: " << grade.grade;
        }
        cout << "\n";
    }
}

// แสดงข้อมูลนักเรียนทั้งหมด
void showStudents(const vector<Student>& students) {
    if (students.empty()) {
        cout << "\nยังไม่มีข้อมูลนักเรียน\n";
        return;
    }

    cout << "\n========== ข้อมูลนักเรียนทั้งหมด ==========\n";
    cout << "จำนวนนักเรียน: " << students.size() << " คน\n";
    for (const Student& student : students) {
        showStudent(student);
        cout << "------------------------------------------\n";
    }
}

// ตรวจว่าข้อความมีตัวคั่นไฟล์หรือไม่ เพื่อป้องกันข้อมูลแยกฟิลด์ผิดรูปแบบ
void hasSeparator(const string& value, bool& found) {
    found = value.find('|') != string::npos;
}

// รับคะแนนรายวิชาทีละวิชาให้กับนักเรียนที่เลือก
void enterSubjectGrades(vector<Student>& students, size_t studentIndex) {
    Student& student = students[studentIndex];
    cout << "\nกรอกชื่อวิชาและคะแนนเต็ม 100 ทีละวิชา\n";
    cout << "เกณฑ์การคำนวณเกรด: " << GRADE_SCALE << "\n";
    cout << "กด Enter ที่ชื่อวิชาเพื่อจบการบันทึก\n";

    while (true) {
        string subject;
        cout << "ชื่อวิชา: ";
        getline(cin, subject);
        trim(subject);
        if (subject.empty()) {
            return;
        }
        bool containsSeparator = false;
        hasSeparator(subject, containsSeparator);
        if (containsSeparator) {
            cout << "ชื่อวิชาห้ามมีเครื่องหมาย | กรุณากรอกใหม่\n";
            continue;
        }

        string scoreText;
        int score = 0;
        bool validScore = false;
        cout << "คะแนนวิชา " << subject << " (0-100): ";
        getline(cin, scoreText);
        parseScore(scoreText, score, validScore);
        if (!validScore) {
            cout << "คะแนนไม่ถูกต้อง กรุณากรอกจำนวนเต็มตั้งแต่ 0 ถึง 100\n";
            continue;
        }

        // ถ้ามีวิชานี้แล้วให้แทนคะแนนเดิม มิฉะนั้นเพิ่มรายการวิชาใหม่
        bool updated = false;
        for (SubjectGrade& existing : student.grades) {
            if (existing.subject == subject) {
                existing.score = score;
                existing.hasScore = true;
                calculateGrade(score, existing.grade);
                updated = true;
                break;
            }
        }
        if (!updated) {
            SubjectGrade newSubjectGrade;
            newSubjectGrade.subject = subject;
            newSubjectGrade.score = score;
            newSubjectGrade.hasScore = true;
            calculateGrade(score, newSubjectGrade.grade);
            student.grades.push_back(newSubjectGrade);
        }

        saveStudents(students);
        string grade;
        calculateGrade(score, grade);
        cout << (updated ? "ปรับปรุงคะแนนวิชาเรียบร้อยแล้ว เกรดที่ได้: "
                         : "บันทึกคะแนนวิชาเรียบร้อยแล้ว เกรดที่ได้: ")
             << grade << "\n";
    }
}

// เพิ่มข้อมูลนักเรียน
void addStudent(vector<Student>& students) {
    Student student;

    cout << "กรอกรหัสนักเรียน: ";
    getline(cin, student.id);
    trim(student.id);
    bool containsSeparator = false;
    hasSeparator(student.id, containsSeparator);
    if (student.id.empty() || containsSeparator) {
        cout << "กรุณากรอกรหัสนักเรียนให้ถูกต้อง\n";
        return;
    }
    // รหัสที่มีอยู่แล้วจะพาไปเพิ่มหรือแก้ไขเกรด แทนการสร้างนักเรียนซ้ำ
    size_t existingIndex = 0;
    findStudent(students, student.id, existingIndex);
    if (existingIndex != students.size()) {
        cout << "พบนักเรียนรหัสนี้แล้ว สามารถเพิ่มหรือแก้ไขคะแนนรายวิชาได้\n";
        enterSubjectGrades(students, existingIndex);
        return;
    }

    cout << "กรอกชื่อ-นามสกุล: ";
    getline(cin, student.name);
    trim(student.name);

    cout << "กรอกห้องเรียน: ";
    getline(cin, student.room);
    trim(student.room);

    bool invalidName = false;
    bool invalidRoom = false;
    hasSeparator(student.name, invalidName);
    hasSeparator(student.room, invalidRoom);
    if (student.name.empty() || student.room.empty() ||
        invalidName || invalidRoom) {
        cout << "กรุณากรอกชื่อและห้องเรียนให้ถูกต้อง "
                "(ห้ามใช้เครื่องหมาย |)\n";
        return;
    }

    students.push_back(student);
    enterSubjectGrades(students, students.size() - 1);
    saveStudents(students);
}

// แก้ไขข้อมูลพื้นฐานและเพิ่มหรือปรับปรุงคะแนนรายวิชาของนักเรียนด้วยรหัส
void editStudent(vector<Student>& students) {
    showStudents(students);
    if (students.empty()) {
        return;
    }

    string id;
    cout << "กรอกรหัสนักเรียนที่ต้องการแก้ไข: ";
    getline(cin, id);

    trim(id);
    size_t index = 0;
    findStudent(students, id, index);
    if (index == students.size()) {
        cout << "ไม่พบรหัสนักเรียนนี้\n";
        return;
    }

    Student& student = students[index];
    string newName;
    string newRoom;
    cout << "ชื่อเดิม: " << student.name << "\n";
    cout << "กรอกชื่อใหม่ (Enter เพื่อคงชื่อเดิม): ";
    getline(cin, newName);
    cout << "ห้องเดิม: " << student.room << "\n";
    cout << "กรอกห้องใหม่ (Enter เพื่อคงห้องเดิม): ";
    getline(cin, newRoom);

    trim(newName);
    trim(newRoom);
    bool invalidName = false;
    bool invalidRoom = false;
    hasSeparator(newName, invalidName);
    hasSeparator(newRoom, invalidRoom);
    if ((!newName.empty() && invalidName) ||
        (!newRoom.empty() && invalidRoom)) {
        cout << "ห้ามใช้เครื่องหมาย | ในชื่อหรือห้องเรียน\n";
        return;
    }

    // ช่องที่เว้นว่างคงค่าเดิมไว้ แล้วบันทึกข้อมูลก่อนเปิดขั้นตอนแก้ไขคะแนน
    if (!newName.empty()) student.name = newName;
    if (!newRoom.empty()) student.room = newRoom;
    saveStudents(students);
    cout << "แก้ไขข้อมูลเรียบร้อยแล้ว\n";

    // อนุญาตให้แก้คะแนนวิชาเดิมหรือเพิ่มคะแนนวิชาใหม่; กด Enter เพื่อข้ามหรือจบ
    enterSubjectGrades(students, index);
}

// ลบข้อมูลนักเรียน
void deleteStudent(vector<Student>& students) {
    showStudents(students);
    if (students.empty()) {
        return;
    }

    string id;
    cout << "กรอกรหัสนักเรียนที่ต้องการลบ: ";
    getline(cin, id);
    trim(id);
    size_t index = 0;
    findStudent(students, id, index);
    if (index == students.size()) {
        cout << "ไม่พบรหัสนักเรียนนี้\n";
        return;
    }

    cout << "ยืนยันลบ " << students[index].name << " (y/n): ";
    string confirm;
    getline(cin, confirm);
    // ลบข้อมูลเมื่อผู้ใช้ยืนยันด้วย y หรือ Y เท่านั้น
    if (confirm == "y" || confirm == "Y") {
        students.erase(students.begin() + index);
        saveStudents(students);
        cout << "ลบข้อมูลเรียบร้อยแล้ว\n";
    } else {
        cout << "ยกเลิกการลบ\n";
    }
}

// ตั้งค่าการแสดงผลภาษาไทย โหลดข้อมูล และวนรับคำสั่งจากเมนูหลัก
int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    vector<Student> students;
    loadStudents(students);
    cout << "ระบบจัดการข้อมูลนักเรียนและเกรดรายวิชา (C++)\n";

    // เรียกฟังก์ชันตามเมนูที่เลือก และจบการทำงานเมื่อเลือก 0
    while (true) {
        cout << "\n===== เมนูหลัก =====\n";
        cout << "1. แสดงข้อมูลนักเรียนและเกรดทั้งหมด\n";
        cout << "2. เพิ่มข้อมูลนักเรียน\n";
        cout << "3. แก้ไขข้อมูลนักเรียน\n";
        cout << "4. ลบข้อมูลนักเรียน\n";
        cout << "0. ออกจากระบบ\n";
        cout << "เลือกเมนู: ";

        string choice;
        getline(cin, choice);

        if (choice == "1") {
            showStudents(students);
        } else if (choice == "2") {
            addStudent(students);
        } else if (choice == "3") {
            editStudent(students);
        } else if (choice == "4") {
            deleteStudent(students);
        } else if (choice == "0") {
            saveStudents(students);
            cout << "ออกจากระบบแล้ว\n";
            break;
        } else {
            cout << "กรุณาเลือกเมนู 0-4\n";
        }
    }

    return 0;
}
