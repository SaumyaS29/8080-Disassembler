#include <iostream>
#include <fstream>
#include <unordered_map>

int BUFFERSIZE=0;
using namespace std;
unsigned int lookahead = 0;
char* buffer;     //pointer to buffer for the file

/*This hashtable maps 1 byte instruction opcodes to its mnemonic.
Note however that some 1 byte instructions are not mentioned here,
because they encode the register operands as well and so they cannot be directly mapped
like these instructions,and they will be dealt with separately*/
unordered_map<unsigned char, string>bytes1_8080 = 
{
	{0x00,"NOP"},//No operation
	{0x02,"STAX B"},
	{0x12,"STAX D"},
	{0x0A,"LDAX B"},
	{0x1A,"LDAX D"},
	{0x3F,"CMC"}, //complement carry
	{0x37,"STC"}, //set carry
	{0x2F,"CMA"}, //complement accumulator(A)
	{0x27,"DAA"}, //Decimal adjust accumulator
	{0x07,"RLC"}, //Rotate accumulator left
	{0x0F,"RRC"},  //Rotate accumulator right
	{0x17,"RAL"},   //Rotate accumulator left through carry
	{0x1F,"RAR"},   //Rotate accumulator right through carry
					//Stack push operations
	{0xC5,"PUSH B"},
	{0xD5,"PUSH D"},
	{0xE5,"PUSH H"},
	{0xF5,"PUSH PSW"},
					//Stack pop operations
	{0xC1,"POP B"},
	{0xD1,"POP D"},
	{0xE1,"POP H"},
	{0xF1,"POP PSW"},

					//Some special instructions
	{0x09,"DAD B"},  //Double addition
	{0x19,"DAD D"},
	{0x29,"DAD H"},
	{0x39,"DAD SP"},

	{0x03,"INX B"},
	{0x13,"INX D"},
	{0x23,"INX H"},
	{0x33,"INX SP"},

	{0x0B,"DCX B"},
	{0x1B,"DCX D"},
	{0x2B,"DCX H"},
	{0x3B,"DCX SP"},

	{0xEB,"XCHG"},
	{0xE3,"XTHL"},
	{0xF9,"SPHL"},
	{0x76,"HLT"},

	{0xC9,"RET"},
	{0xD8,"RC"},
	{0xD0,"RNC"},
	{0xC8,"RZ"},
	{0xC0,"RNZ"},
	{0xF0,"RP"},
	{0xF8,"RM"},
	{0xE8,"RPE"},
	{0xE0,"RPO"},

	{0xFB,"EI"},
	{0xF3,"DI"},
	{0xE9,"PCHL"}
};

unordered_map <unsigned char, string>bytes2_8080 =
{
	//Accumulator based immediate operation instructions
	{0xC6,"ADI"},
	{0xCE,"ACI"},
	{0xD6,"SUI"},
	{0xDE,"SBI"},
	{0xE6,"ANI"},
	{0xEE,"XRI"},
	{0xF6,"ORI"},
	{0xFE,"CPI"},

	{0xDB,"IN"},
	{0xD3,"OUT"}
};

unordered_map<unsigned char, string>bytes3_8080 =
{
	{0x01,"LXI B"},
	{0x11,"LXI D"},
	{0x21,"LXI H"},
	{0x31,"LXI SP"},
	{0x3A,"LDA"},  //Load accumulator direct
	{0x22,"SHLD"},
	{0x32,"STA"},
	{0x2A,"LHLD"},
	{0xC3,"JMP"},
	{0xDA,"JC"},
	{0xD2,"JNC"},
	{0xCA,"JZ"},
	{0xC2,"JNZ"},
	{0xFA,"JM"},
	{0xF2,"JP"},
	{0xEA,"JPE"},
	{0xE2,"JPO"},
	{0xCD,"CALL"},
	{0xDC,"CC"},
	{0xD4,"CNC"},
	{0xCC,"CZ"},
	{0xC4,"CNZ"},
	{0xFC,"CM"},
	{0xF4,"CP"},
	{0xEC,"CPE"},
	{0xE4,"CPO"}
};
/*This hashmap maps register codes to their corresponding names*/
unordered_map<unsigned char, char> register_table8080 = 
{
	{0,'B'},
	{1,'C'},
	{2,'D'},
	{3,'E'},
	{4,'H'},
	{5,'L'},
	{6,'M'}, //Memory reference
	{7,'A'}
};

unordered_map<unsigned char, string>arithmetic_instr =
{
	{0,"ADD"},
	{1,"ADC"},
	{2,"SUB"},
	{3,"SBB"},
	{4,"ANA"},
	{5,"XRA"},
	{6,"ORA"},
	{7,"CMP"}
};

unsigned char get_byte()
{
	unsigned char ch = buffer[lookahead]; 
	lookahead++;
	return ch;
}
void disassemble_line()
{
	printf("0x%x: ", lookahead);
	unsigned char bytecode = get_byte();
	if (bytes1_8080.find(bytecode) != bytes1_8080.end()) {
		cout << bytes1_8080[bytecode] << endl;
	}
	else if (bytes2_8080.find(bytecode) != bytes2_8080.end())
	{
		cout << bytes2_8080[bytecode];
		bytecode = get_byte();
		printf(" 0x%x\n",bytecode);
	}
	else if (bytes3_8080.find(bytecode) != bytes3_8080.end())
	{
		cout << bytes3_8080[bytecode];
		if (bytecode == 0x01 || bytecode == 0x11 || bytecode == 0x21 || bytecode == 0x31) {
			cout << " , ";
		}
		else cout << " &";
		bytecode = get_byte(); //Low byte
		unsigned int addr = (unsigned int)bytecode;
		bytecode = get_byte(); //high byte
		unsigned int high = ((unsigned int)bytecode) << 8;
		addr = addr + high;
		printf("0x%x\n",addr);
	}
	else {  //Here we deal with instructions that need bitmasking
		unsigned char mask = ((bytecode & 0xC0) >> 6),dest,src,test;
		switch (mask) {
		case 0:  //Move immediate data(2) and increment, decrement(1)
			test = (bytecode & 0x07);
			if (test == 4) {
				cout << "INR ";
				src = (bytecode >> 3) & 0x07;
				cout << register_table8080[src] << endl;
				break;
			}
			else if (test == 5) {
				cout << "DCR ";
				src = (bytecode >> 3) & 0x07;
				cout << register_table8080[src] << endl;
				break;
			}
			cout << "MVI ";
			dest = (bytecode & 0x38) >> 3;
			cout << register_table8080[dest] << " , ";
			bytecode = get_byte();
			printf("0x%x\n", bytecode);
			break;
		case 1:  //MOV reg-reg 1 byte instruction
			cout << "MOV ";
			dest = ((bytecode & 0x38) >> 3);
			cout << register_table8080[dest] << " , ";
			src = bytecode & 0x07;
			cout << register_table8080[src] << endl;
			break;
		case 2:   //register-accumulator arithmetic

			dest = ((bytecode & 0x38) >> 3);
			cout << arithmetic_instr[dest] << " ";
			src = bytecode & 0x07;
			cout << register_table8080[src] << endl;
			break;
		case 3:    //Restart instruction
			dest = (bytecode & 0x07);
			if (dest == 3) {
				cout << "RST ";
				test = (bytecode >> 3) & 0x07;
				if(test < 8)printf("0x%x\n", test);
			}
			break;
		}
	}

}
int main()
{
	string filepath;
	fstream file;
	cout << "Enter ROM file path: ";
	cin >> filepath;
	
	file.open(filepath, std::ios_base::in | std::ios_base::binary);
	
	//Get file size
	file.seekg(0, file.end);
	BUFFERSIZE = file.tellg();
	file.seekg(0, file.beg);
	
	buffer = new char[BUFFERSIZE];
	file.read(buffer,BUFFERSIZE);
	cout << "Disassembly of file at: " << filepath << endl;
	while(lookahead < BUFFERSIZE)
	{
		disassemble_line();
	}
	file.close();
	delete[] buffer;
	return 0;
}