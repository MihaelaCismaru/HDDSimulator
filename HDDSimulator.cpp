#pragma warning (disable : 4996)
#include <iostream>
#include <string>
#include <bitset>
#include <algorithm>
using namespace std;

int result[16];
int DimFAT, DimRoot, cellsFATUI;

string HDD[4096];

string command;
string input[4];
string aux;

struct File {
	string name, extension;
	int size, type;
};

string NumberToBytes(int nr, int nrOfBytes) {
	//This method receives a number and transforms it in a string of bits of size nrOfBytes*2
	aux.clear();
	for (int i = 1; i <= nrOfBytes * 8; i++) {
		aux.push_back((nr % 2) + '0');
		nr /= 2;
	}
	for (int i = 1; i <= aux.size() / 2; i++) {
		swap(aux[i - 1], aux[aux.size() - i]);
	}
	return aux;
}

void BytesToNumbers(string x) {
	/// this method takes the row with index x in the HDD table and coverts each byte in a number.
	/// These numbers end up in the result array
	for (int i = 0, k = 0; i < 128; i += 8, k++) {
		result[k] = 0;
		for (int j = i; j < i + 8; j++) {
			result[k] *= 2;
			result[k] += x[j] - '0';
		}
	}
}

File GetFileDataFromRoot(string HDDrow) {
	/// this method converts the bits stored in a HDD row (a row with root data) into the name,extension,
	/// size and type of a file
	BytesToNumbers(HDDrow);
	string name = "";
	for (int j = 0; j < 8; j++) {
		if ((char)result[j] != 0) {
			name += (char)result[j];
		}
	}
	string extension = "";
	for (int j = 8; j < 11; j++) {
		if ((char)result[j] != 0) {
			extension += (char)result[j];
		}
	}
	int size = result[11] * 256 + result[12];
	int type = result[15];
	return { name, extension, size, type };
}

void ShowFileData(int showSize) {
	/// This method calls the method that converts the stored data into file information
	/// then prints it in the console (logic for command DIR)
	for (int i = DimFAT; i < DimFAT + DimRoot; i++) {
		if (HDD[i] == "") {
			continue;
		}
		File aux = GetFileDataFromRoot(HDD[i]);
		if (showSize == 0)
			cout << aux.name << "." << aux.extension << "\n";
		else {
			cout << aux.name << "." << aux.extension << " " << aux.size << " ";
			if (aux.type == 0) {
				cout << "-ALFA\n";
			}
			if (aux.type == 1) {
				cout << "-NUM\n";
			}
			if (aux.type == 2) {
				cout << "-HEX\n";
			}
		}
	}
}

int FindEmptyUI() {
	/// This method goes through every pair of two consecutive bytes in the FAT part of the HDD table
	/// searching for a row that is marked in the HDD table with 0 meaning that is free. The iterations starts
	/// after the reserved space for FAT and ROOT, just to don't search for nothing there
	int cnt = 575;
	for (int i = 72; i < 512; i++) {
		BytesToNumbers(HDD[i]);
		for (int j = 0; j < 16; j += 2) {
			cnt++;
			if (result[j] * 128 + result[j + 1] == 0) {
				return cnt;
			}
		}
	}
	return -1;
}

void ChangeFATValue(int index, int value) {
	/// This method change the pair of 2 bytes that correspond to the HDD row with this index
	/// a set the new value for it (everying is converted in bits)
	int i = index / 8;
	int j = (index % 8) * 16;
	string newString = NumberToBytes(value, 2);
	for (int k = 0; k < 16; k++) {
		HDD[i][j + k] = newString[k];
	}
}

int GetFatValue(int index) {
	/// This method gets the value stored in the FAT table for the HDD row with this index
	/// converts it from bits to number then returns it
	int i = index / 8;
	int j = (index % 8) * 16;
	int nr = 0;
	for (int k = 0; k < 16; k++) {
		nr *= 2;
		nr += HDD[i][j + k] - '0';
	}
	return nr;
}

void StoreFile(int FirstUI, int size, int type) {
	/// This method stores the file data and marks the FAT table
	int neededUI = size;
	int group = 1;
	type == 0 ? group = 16 : group = 32;
	neededUI /= group;
	if(size % group != 0) neededUI++;
	
	int myUI = FirstUI;
	char ch = 'a'; int nr = 0;
	for (int i = 1; i <= neededUI; i++) {
		for (int j = 1; j <= 16; j++) {
			if (type == 0) {
				HDD[myUI] += NumberToBytes(ch, 1);
				(ch == 'z') ? ch = 'a' : ch++;
			}
			else {
				HDD[myUI] += NumberToBytes(nr, 1);
				((nr == 9 && type == 1) || (nr == 15 && type == 2)) ? nr = 0 : nr++;
			}	
		}
		if (i != neededUI) {
			ChangeFATValue(myUI, 4);
			int nextUI = FindEmptyUI();
			if (nextUI == -1) {
				cout << "There is no more room to save the whole file!\n";
				ChangeFATValue(myUI, 3);
				break;
			}
			ChangeFATValue(myUI, nextUI);
			myUI = nextUI;
		}
		else {
			ChangeFATValue(myUI, 3);
		}
	}
}

int FindEmptyRowInRoot() {
	/// This method searches for a empty space in the Root parts of the HDD table to store a new file
	/// If it finds an empty spot it return its index
	int index = -1;
	for (int i = DimFAT; i < DimFAT + DimRoot; i++) {
		if (HDD[i] == "") {
			index = i;
			break;
		}
	}
	return index;
}

void CreateNewFile(string givenName, string givenExtension, int givenSize, int fileType) {
	/// This method is the main logic for the CREATE command, it converts the data to bits and populates
	/// its data row in the root part, fills row with the content converted in bits, and marks the fat table according
	int index = FindEmptyRowInRoot();
	if (index == -1) {
		cout << "There is no more space for a new file!\n";
		return;
	}
	for (int i = 0; i < givenName.size() && i < 8; i++) {
		HDD[index] += NumberToBytes(givenName[i], 1);
	}
	for (int i = givenName.size(); i < 8; i++) {
		HDD[index] += NumberToBytes(0, 1);
	}
	for (int i = 0; i < 3; i++) {
		HDD[index] += NumberToBytes(givenExtension[i], 1);
	}
	HDD[index] += NumberToBytes(givenSize, 2);
	int firstUI = FindEmptyUI();
	if (firstUI == -1) {
		cout << "There is no more room for saving this file!\n";
		HDD[index] = "";
		return;
	}
	HDD[index] += NumberToBytes(firstUI, 2);
	HDD[index] += NumberToBytes(fileType, 1);
	StoreFile(firstUI, givenSize, fileType);
}

void CopyContent(int originalUI, int myUI) {
	///This method copies the content stored for a file into a new set of rows for a new file.
	while (true) {
		HDD[myUI] = HDD[originalUI];

		int nextOriginalUI = GetFatValue(originalUI);
		if (nextOriginalUI == 3) {
			ChangeFATValue(myUI, 3);
			break;
		}

		ChangeFATValue(myUI, 4);
		int nextMyUI = FindEmptyUI();
		if (nextMyUI == -1) {
			cout << "There is no more room to save the whole file!\n";
			ChangeFATValue(myUI, 3);
			break;
		}
		ChangeFATValue(myUI, nextMyUI);

		myUI = nextMyUI;
		originalUI = nextOriginalUI;
	}
}

int FindFileName(string wantedName, string wantedExtension) {
	///This method searches the root part of the HDD to verify if a file with this name already exists in the HDD
	for (int i = DimFAT; i < DimFAT + DimRoot; i++) {
		if (HDD[i] == "") {
			continue;
		}
		File aux = GetFileDataFromRoot(HDD[i]);
		if (aux.name == wantedName && aux.extension == wantedExtension) {
			return i;
		}
	}
	return -1;
}

void DeleteFile(int index) {
	/// This method deletes everything related to a file, the fields return to the state before its existence
	BytesToNumbers(HDD[index]);
	int myUI = result[13] * 256 + result[14];
	while (true) {
		int nextUI = GetFatValue(myUI);
		ChangeFATValue(myUI, 0);
		HDD[myUI] = "";
		if (nextUI == 3) {
			break;
		}
		myUI = nextUI;
	}
	HDD[index] = "";
}

void RenameFile(int index, string name, string extension) {
	/// This method changes the name of a file
	for (int i = 0; i < name.size() && i < 8; i++) {
		string aux = NumberToBytes(name[i], 1);
		for (int j = 0; j < 8; j++) {
			HDD[index][i * 8 + j] = aux[j];
		}
	}
	for (int i = name.size(); i < 8; i++) {
		for (int j = 0; j < 8; j++) {
			HDD[index][i * 8 + j] = '0';
		}
	}
	for (int i = 0; i < 3; i++) {
		string aux = NumberToBytes(extension[i], 1);
		for (int j = 0; j < 8; j++) {
			HDD[index][i * 8 + j + 64] = aux[j];
		}
	}
}

void CopyFile(int index, string name, string extension) {
	/// This method has the main logic for the COPY command
	int newIndex = FindEmptyRowInRoot();
	if (newIndex == -1) {
		cout << "There is no more space for a new file!\n";
		return;
	}
	HDD[newIndex] = HDD[index];
	
	RenameFile(newIndex, name, extension);

	BytesToNumbers(HDD[index]);
	int originalUI = result[13] * 256 + result[14];
	
	int firstUI = FindEmptyUI();

	if (firstUI == -1) {
		cout << "There is no more room for copying this file!\n";
		HDD[newIndex] = "";
		return;
	}

	string aux = NumberToBytes(firstUI, 2);
	for (int i = 13 * 8, k = 0; i < 15 * 8; i++, k ++) {
		HDD[newIndex][i] = aux[k];
	}
	CopyContent(originalUI, firstUI);
}

pair<string,string> ValidateFileName(string input) {
	/// This method validates the input when a file name is expected and converts it to two
	/// strings representing the name and the extention
	string name = "";
	string extension = "";
	int ext = 0;
	for (int j = 0; j < input.size(); j++) {
		if (input[j] == '.') {
			ext = 1;
			continue;
		}
		if (ext == 1) {
			extension += input[j];
		}
		else {
			name += input[j];
		}
	}
	if (ext == 0) {
		cout << "File must have an extension!\n";
		return {"",""};
	}
	if (name.size() == 0 || name.size() > 8) {
		cout << "File name size is invalid\n";
		return {"",""};
	}
	if (extension.size() != 3) {
		cout << "Extension size is invalid\n";
		return {"",""};
	}
	return {name,extension};
}

int ValidateFileSize(string input) {
	/// This method validates the input when the size of the file is expected
	int marime = 0;
	int marimeOK = 1;
	for (int j = 0; j < input.size(); j++) {
		marime *= 10;
		if (input[j] > '9' || input[j] < '0') {
			marimeOK = 0;
			break;
		}
		marime += input[j] - '0';
	}
	if (marimeOK == 0) {
		cout << "Invalid file size\n";
		return -1;
	}
	if (marime > 20000) {
		cout << "File too large\n";
		return -1;
	}
	return marime;
}

void InitializeFATTable() {
	//populam tabela FAT pentru rezervarea spatiului FAT
	for (int i = 0; i < 64; i++) {
		HDD[i] = "";
		for (int j = 1; j <= cellsFATUI; j++) {
			HDD[i] += NumberToBytes(1, 2);
		}
	}

	//populam tabela FAT pentru rezervarea spatiului ROOT
	for (int i = 64; i < 72; i++) {
		HDD[i] = "";
		for (int j = 1; j <= cellsFATUI; j++) {
			HDD[i] += NumberToBytes(2, 2);
		}
	}

	//populam tabela FAT pentru notarea spatilor libere
	for (int i = 72; i < 512; i++) {
		HDD[i] = "";
		for (int j = 1; j <= cellsFATUI; j++) {
			HDD[i] += NumberToBytes(0, 2);
		}
	}
}

string ToUpperCase(string x) {
	///This method converts a string to only have uppercase letters
	for (int i = 0; i < x.size(); i++) {
		if (x[i] >= 'a' && x[i] <= 'z') {
			x[i] -= ('a' - 'A');
		}
	}
	return x;
}

int main()
{
	cout << "Hello World!\n";

	DimFAT = 512;
	DimRoot = 64;
	cellsFATUI = 8;

	InitializeFATTable();
	cout << "Welcome to HDD simulator, type HELP for instructions!\n";
	while (true) {
		cout << "My_OS>";
		string command;
		getline(cin, command);

		int cntInp = 0;
		int canChange = 1;
		pair<string, string> nameAux;

		for (int i = 0; i < 4; i++) {
			input[i].clear();
		}

		for (int i = 0; i < command.size(); i++) {
			if (command[i] == ' ' && canChange == 1) {
				canChange = 0;
				cntInp++;
				if (cntInp == 4) {
					break;
				}
			}
			if (command[i] != ' ') {
				canChange = 1;
				input[cntInp].push_back(command[i]);
			}
		}

		input[0] = ToUpperCase(input[0]);

		if (input[0].empty() || !(input[0] == "DIR" || input[0] == "CREATE" || input[0] == "DELETE" || input[0] == "RENAME" || input[0] == "COPY" || input[0] == "HELP")) {
			cout << "Invalid command!" << "\n";
			continue;
		}

		if (input[0] == "HELP") {
			cout << "To create a file: CREATE fileName.ext 100 -HEX\n ---filename 8ch max, extension exactly 3ch, type can have values: -ALFA,-NUM,-HEX, size between 1 and 20000\n";
			cout << "To delete a file: DELETE fileName.ext\n";
			cout << "To rename a file: RENAME oldName.ext newName.ext\n";
			cout << "To copy a file: COPY fileName.ext newFile.ext\n";
			cout << "To show all files name: DIR\n";
			cout << "To show all files name, size and type: DIR -a\n";
		}

		if (input[0] == "DIR") {
			if (input[1] == "-a") {
				ShowFileData(1);
			}
			else {
				ShowFileData(0);
			}
		}
		if (input[0] == "CREATE") {
			
			nameAux = ValidateFileName(input[1]);
			string name = nameAux.first;
			string extension = nameAux.second;
			if (name == "") {
				continue;
			}

			int size = ValidateFileSize(input[2]);
			if (size == -1) {
				continue;
			}

			int type = -1;
			if (input[3] == "-ALFA") { type = 0; }
			if (input[3] == "-NUM") { type = 1; }
			if (input[3] == "-HEX") { type = 2; }
			if (type == -1) {
				cout << "Invalid file type\n";
				continue;
			}

			if (FindFileName(name, extension) != -1) {
				cout << "File already exist\n";
				continue;
			}
			CreateNewFile(name, extension, size, type);
		}
		if (input[0] == "DELETE") {
			nameAux = ValidateFileName(input[1]);
			string name = nameAux.first;
			string extension = nameAux.second;
			if (name == "") {
				continue;
			}

			int index = FindFileName(name, extension);
			if (index == -1) {
				cout << "File doesn't exist!\n";
				continue;
			}
			DeleteFile(index);
		}
		if (input[0] == "RENAME" || input[0] == "COPY") {
			nameAux = ValidateFileName(input[1]);
			string initialName = nameAux.first;
			string initialExtension = nameAux.second;
			if (initialName == "") {
				continue;
			}

			nameAux = ValidateFileName(input[2]);
			string newName = nameAux.first;
			string newExtension = nameAux.second;
			if (newName == "") {
				continue;
			}

			int index = FindFileName(initialName, initialExtension);
			if (index == -1) {
				cout << "File doesn't exist\n";
				continue;
			}
			if (FindFileName(newName, newExtension) != -1) {
				cout << "File with this name already exists\n";
				continue;
			}

			if (input[0] == "RENAME") {
				if (newName == initialName && newExtension == initialExtension) {
					continue;
				}
				RenameFile(index, newName, newExtension);
			}
			if (input[0] == "COPY") {
				if (newName == initialName && newExtension == initialExtension) {
					cout << "File with this name already exists!\n";
					continue;
				}
				CopyFile(index, newName, newExtension);
			}
		}
	}
}

