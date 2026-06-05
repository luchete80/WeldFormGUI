/*************************************************************************/
/*  NastranReader.cpp                                            */
/*  WeldformFEM - High-Performance Explicit & Implicit FEM Solvers     */
/*  (CPU/GPU, C++/CUDA)                                                  */
/*                                                                       */
/*  weldform.sph@gmail.com                                                */
/*  ('https://www.opensourcemech.com',)                                    */
/*                                                                       */
/*  Copyright (c) 2023-2025 Luciano Buglioni          */
/*                                                                       */
/*  This file is part of the WeldformFEM project.                     */
/*  Licensed under the GNU General Public License v3.0 or later. */ 
/*  See the LICENSE file in the project    */
/*  root for full license information.                                   */
/*************************************************************************/




#include "NastranReader.h"


//NOT NECESSARY
inline std::string trimSpaces(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        if (c != ' ') out.push_back(c);
    }
    return out;
}

inline bool startsWithCard(const std::string& line, const char* card) {
    const std::string prefix = trimSpaces(line.substr(0, 8));
    return prefix == std::string(card);
}

inline std::string trimWhitespace(const std::string& s) {
    const size_t first = s.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return "";
    }
    const size_t last = s.find_last_not_of(" \t\r\n");
    return s.substr(first, last - first + 1);
}

inline bool startsWithCardPrefix(const std::string& line, const char* card_prefix) {
    const std::string prefix = trimSpaces(line.substr(0, 8));
    return prefix.rfind(std::string(card_prefix), 0) == 0;
}

inline bool isTriCard(const std::string& line) {
    return startsWithCard(line, "CTRIA") || startsWithCard(line, "CTRIA3");
}

inline bool isQuadCard(const std::string& line) {
    return startsWithCard(line, "CQUAD") || startsWithCard(line, "CQUAD4");
}

inline bool isBeamCard(const std::string& line) {
    return startsWithCard(line, "CBEAM") || startsWithCard(line, "CBAR");
}

inline double parseNastranRealField(std::string field) {
    field = trimSpaces(field);
    if (field.empty()) {
        return 0.0;
    }

    for (auto &c : field) {
        if (c == 'D' || c == 'd') c = 'E';
    }

    if (field.find('E') == std::string::npos &&
        field.find('e') == std::string::npos) {
        for (size_t k = 1; k < field.size(); ++k) {
            if ((field[k] == '+' || field[k] == '-') &&
                std::isdigit(static_cast<unsigned char>(field[k - 1]))) {
                field.insert(k, "E");
                break;
            }
        }
    }

    return strtod(field.c_str(), nullptr);
}

inline int parseNastranIntField(const std::string& line, int fieldIndex)
{
    const int pos = fieldIndex * FIELD_LENGTH;
    if (pos < 0 || pos + FIELD_LENGTH > static_cast<int>(line.size())) {
        return 0;
    }
    return atoi(line.substr(pos, FIELD_LENGTH).c_str());
}

inline bool isContinuationLine(const std::string& raw_line) {
    const std::string first = trimWhitespace(raw_line.substr(0, FIELD_LENGTH));
    return first.empty() || first == "+" || first == "*";
}

inline std::vector<int> readChexaNodeIds(const std::vector<std::string>& rawData,
                                         int line_index,
                                         int line_count) {
    std::vector<int> node_ids;
    for (int field = 3; field <= 8 && node_ids.size() < 6; ++field) {
        const int id = parseNastranIntField(rawData[line_index], field);
        if (id != 0) {
            node_ids.push_back(id);
        }
    }

    for (int next = line_index + 1;
         next < line_count && node_ids.size() < 8 && isContinuationLine(rawData[next]);
         ++next) {
        for (int field = 1; field <= 8 && node_ids.size() < 8; ++field) {
            const int id = parseNastranIntField(rawData[next], field);
            if (id != 0) {
                node_ids.push_back(id);
            }
        }
    }

    return node_ids;
}

void NastranReader::read(const char* fName){
	string fileName = fName;
  string line;
  //rawData="";
	fstream file;
    bool found=false;
	//MPI_Comm_rank(MPI_COMM_WORLD, &Rank);
	cout << "[I] Reading ... "<<endl;
	file.open(fileName.c_str());
	if (file.is_open()) {
		//cout << "[I] Found input file " << fileName << endl;
		found=true;
	} else {
		//cerr << "[E] Input file " << fileName << " could not be found!!" << endl;
	}
	
  int l=0;
  node_count = 0;
  elem_count = 0;
  
  dim = 3;
  max_nodes_per_elem = 3;
  bool found_ctetra = false;
  bool found_chexa = false;
  bool found_shell = false;
  bool found_ctria = false;
  bool found_cquad = false;
  bool found_beam = false;
  
  bool start_node = false;
  bool start_elem = false;
  
  int line_start_node;
	int line_start_elem;
  
	if (found) {	
		while(getline(file, line)) {
      rawData.push_back(line /*+ "\n"*/); 
      if (startsWithCard(line, "GRID")){
        node_count++;
        if (!start_node){
          start_node = true;
          line_start_node = l;
        }
      } else if (startsWithCardPrefix(line, "CTRIA") ||
                 startsWithCardPrefix(line, "CQUAD") ||
                 startsWithCard(line, "CTETRA") ||
                 startsWithCardPrefix(line, "CHEXA") ||
                 startsWithCardPrefix(line, "CBEAM") ||
                 startsWithCardPrefix(line, "CBAR") ){
        if (startsWithCardPrefix(line, "CHEXA"))
          found_chexa = true;
        else if (startsWithCard(line, "CTETRA"))
          found_ctetra = true;
        else if (startsWithCardPrefix(line, "CTRIA") || startsWithCardPrefix(line, "CQUAD")) {
          found_shell = true;
          if (startsWithCardPrefix(line, "CTRIA")) found_ctria = true;
          if (startsWithCardPrefix(line, "CQUAD")) found_cquad = true;
        } else if (startsWithCardPrefix(line, "CBEAM") || startsWithCardPrefix(line, "CBAR")) {
          found_beam = true;
        }
      }
      l++;
    }
    file.close();

    file.clear();
    file.open(fileName.c_str());
    if (!file.is_open()) {
      std::cerr << "[E] Failed to reopen Nastran file for parsing: " << fileName << std::endl;
      return;
    }

    rawData.clear();
    l = 0;
    start_node = false;
    start_elem = false;
    node_count = 0;
    elem_count = 0;
    if (found_chexa) {
      dim = 3;
      max_nodes_per_elem = 8;
    } else if (found_ctetra) {
      dim = 3;
      max_nodes_per_elem = 4;
    } else if (found_shell) {
      dim = 2;
      max_nodes_per_elem = found_cquad ? 4 : 3;
    } else {
      dim = 2;
      max_nodes_per_elem = 2;
    }

		while(getline(file, line)) {
      rawData.push_back(line);
      if (startsWithCard(line, "GRID")){
        node_count++;
        if (!start_node){
          start_node = true;
          line_start_node = l;
        }
      } else {
        const bool is_selected_elem =
            (found_chexa && startsWithCardPrefix(line, "CHEXA")) ||
            (!found_chexa && found_ctetra && startsWithCard(line, "CTETRA")) ||
            (!found_chexa && !found_ctetra &&
             (startsWithCardPrefix(line, "CTRIA") ||
              startsWithCardPrefix(line, "CQUAD") ||
              (!found_shell && startsWithCardPrefix(line, "CBEAM")) ||
              (!found_shell && startsWithCardPrefix(line, "CBAR"))));
        if (!is_selected_elem) {
          l++;
          continue;
        }
        if (!start_elem){
          start_elem = true;
					line_start_elem = l;
				}
        if (startsWithCardPrefix(line, "CQUAD"))
          max_nodes_per_elem = 4;
        if (startsWithCardPrefix(line, "CHEXA")) {
          max_nodes_per_elem = 8;
          dim = 3;
        }
        if (startsWithCard(line, "CTETRA")) {
          max_nodes_per_elem = 4;
          dim = 3;
        }
        if (!found_chexa && !found_ctetra &&
            (startsWithCardPrefix(line, "CTRIA") || startsWithCardPrefix(line, "CQUAD")))
          dim = 2;
        if (!found_chexa && !found_ctetra && !found_shell &&
            (startsWithCardPrefix(line, "CBEAM") || startsWithCardPrefix(line, "CBAR")))
          dim = 2;
        elem_count++;
      }
      l++;
    }
		file.close();
    
    cout << node_count <<" nodes and "<<elem_count<< " elements found."<<endl;
    if (found_chexa) {
      cout << "[Nastran] CHEXA detected. Tetra/shell/beam elements are ignored for solid import." << endl;
    } else if (found_ctetra) {
      cout << "[Nastran] CTETRA detected. Shell/beam elements are ignored for solid import." << endl;
    }

		// Strip all the inline or block comments (C++ style) from the rawData
		//stripComments(rawData);
		// Strip all the white spaces from the rawData
		//strip_white_spaces(rawData);
	}
	cout << "[I] "<<l << " lines readed ..." <<endl;
	line_count = l;
  
  //Allocating nodes 
  cout << "Allocating " << node_count<<" nodes"<<endl;
  node  	= new double 	[3 * node_count];
  nodeid  = new int 		[node_count];

	// NODAL FIELD DATA IS: GRID|ID|CP|X1|	
  int curr_line = line_start_node;
	l = curr_line;
	//~ Vec3_t min( 1000., 1000., 1000.);
  //~ Vec3_t max(-1000.,-1000.,-1000.);
	
  int n = 0;
  for (l = curr_line; l < line_count && n < node_count; ++l) {
    if (!startsWithCard(rawData[l], "GRID"))
      continue;

		string temp = rawData[l].substr(FIELD_LENGTH,FIELD_LENGTH);
		nodeid[n] = atoi(temp.c_str());
		nodepos.insert(std::make_pair(atoi(temp.c_str()),n));
		for (int i = 0;i<3;i++) {
			int pos = 3*(FIELD_LENGTH)+ i*FIELD_LENGTH;
			string temp = rawData[l].substr(pos,FIELD_LENGTH);
			double d = parseNastranRealField(temp);
			node[3*n+i] = d;
		}
    n++;
  }

  if (n != node_count) {
    cerr << "[E] Parsed " << n << " GRID entries, expected " << node_count << endl;
  }
	
	//cout << "Min values: "<< min <<endl;
	//cout << "Max values: "<< max <<endl;	
  
  //IF FIXED FIELD
  cout << "Allocating " <<elem_count<<" Elements..."<<endl;
	// ASSUMING NODE IS FROM 1
  elcon = new int [max_nodes_per_elem * elem_count];

	map<int, int>::iterator it;
  curr_line = line_start_elem;
	l = curr_line;
  int ecount = 0;
  for (l = curr_line; l < line_count && ecount < elem_count; ++l) {
    const bool is_selected_elem =
        (found_chexa && startsWithCardPrefix(rawData[l], "CHEXA")) ||
        (!found_chexa && found_ctetra && startsWithCard(rawData[l], "CTETRA")) ||
        (!found_chexa && !found_ctetra &&
         (startsWithCardPrefix(rawData[l], "CTRIA") ||
          startsWithCardPrefix(rawData[l], "CQUAD") ||
          (!found_shell && startsWithCardPrefix(rawData[l], "CBEAM")) ||
          (!found_shell && startsWithCardPrefix(rawData[l], "CBAR"))));
    if (!is_selected_elem) {
      continue;
    }

    int nodes_per_elem = 3;
    if (startsWithCardPrefix(rawData[l], "CHEXA"))
      nodes_per_elem = 8;
    else if (isQuadCard(rawData[l]) || startsWithCard(rawData[l], "CTETRA"))
      nodes_per_elem = 4;
    else if (isBeamCard(rawData[l]))
      nodes_per_elem = 2;

    const bool debugTargetTetra =
      startsWithCard(rawData[l], "CTETRA") && parseNastranIntField(rawData[l], 1) == 3801;
    if (debugTargetTetra) {
      cout << "[debug tetra 3801][reader] line " << l << ": " << rawData[l] << endl;
      cout << "[debug tetra 3801][reader] pid=" << parseNastranIntField(rawData[l], 2)
           << " node ids="
           << parseNastranIntField(rawData[l], 3) << ", "
           << parseNastranIntField(rawData[l], 4) << ", "
           << parseNastranIntField(rawData[l], 5) << ", "
           << parseNastranIntField(rawData[l], 6) << endl;
    }

    std::vector<int> node_ids(nodes_per_elem, 0);
    if (startsWithCardPrefix(rawData[l], "CHEXA")) {
      const std::vector<int> chexa_nodes = readChexaNodeIds(rawData, l, line_count);
      if (chexa_nodes.size() != 8) {
        cerr << "[E] Invalid CHEXA connectivity at Nastran line " << l + 1
             << ": expected 8 node ids, parsed " << chexa_nodes.size() << endl;
        if (l + 1 < line_count && isContinuationLine(rawData[l + 1])) {
          cerr << "    " << rawData[l] << endl;
          cerr << "    " << rawData[l + 1] << endl;
        }
        continue;
      }
      for (int en = 0; en < 8; ++en)
        node_ids[en] = chexa_nodes[en];
    } else {
      for (int en = 0; en < nodes_per_elem; ++en) {
        const int pos = 3 * FIELD_LENGTH + en * FIELD_LENGTH;
        const std::string temp = rawData[l].substr(pos, FIELD_LENGTH);
        node_ids[en] = atoi(temp.c_str());
      }
    }

    for (int en=0;en<nodes_per_elem;en++){
			int d = node_ids[en];
      auto it_nod = nodepos.find(d);
      if (it_nod == nodepos.end()) {
        cerr << "[E] Node id " << d << " referenced by element line " << l
             << " was not found in GRID table." << endl;
        elcon[max_nodes_per_elem*ecount+en] = -1;
        if (debugTargetTetra) {
          cerr << "[debug tetra 3801][reader] missing GRID for node id " << d << endl;
        }
        continue;
      }
			int nod = it_nod->second;
			elcon[max_nodes_per_elem*ecount+en] = nod;
      if (debugTargetTetra) {
        cout << "[debug tetra 3801][reader] node " << d
             << " -> internal index " << nod
             << " coords=("
             << node[3 * nod] << ", "
             << node[3 * nod + 1] << ", "
             << node[3 * nod + 2] << ")" << endl;
      }
		}
    for (int en=nodes_per_elem; en<max_nodes_per_elem; en++)
      elcon[max_nodes_per_elem*ecount+en] = -1;
    if (debugTargetTetra) {
      cout << "[debug tetra 3801][reader] stored connectivity indices="
           << elcon[max_nodes_per_elem*ecount] << ", "
           << elcon[max_nodes_per_elem*ecount + 1] << ", "
           << elcon[max_nodes_per_elem*ecount + 2] << ", "
           << elcon[max_nodes_per_elem*ecount + 3] << endl;
    }
    ecount++;
	}

  if (ecount != elem_count) {
    cerr << "[E] Parsed " << ecount << " element entries, expected " << elem_count << endl;
  }
  cout << "Done."<<endl;
	
}

void NastranReader::WriteCSV(char const * FileKey)
{
	//type definition to shorten coding
	std::ostringstream oss;
	//Writing in a Log file
	//String fn(FileKey);
	
	oss << "X, Y, Z"<<endl;;
	
	//#pragma omp parallel for schedule(static) num_threads(Nproc)
	// #ifdef __GNUC__
	// for (size_t i=0; i<Particles.Size(); i++)	//Like in Domain::Move
	// #else
	for (int i=0; i<node_count; i++)//Like in Domain::Move
	//#endif
	{
		for (int j=0;j<3;j++){
			oss << node[3*i+j];
			if (j<2)	oss <<", ";
		}
		oss <<endl;
		
	}

	//fn = FileKey;
	//fn.append(".csv");	
	//std::ofstream of(fn.CStr(), std::ios::out);
	//of << oss.str();
	//of.close();
}

void NastranReader::WriteVTK(char const * FileKey)
{

	string fileName(FileKey);
	fileName.append(".vtu");	
	ofstream file;
	file.open((fileName).c_str(),ios::out);
	file << "<?xml version=\"1.0\"?>" << endl;
	file << "<VTKFile type=\"UnstructuredGrid\">" << endl;
	file << "<UnstructuredGrid>" << endl;
	file << "<Piece NumberOfPoints=\"" << node_count << "\" NumberOfCells=\"" << elem_count << "\">" << endl;
	file << "<Points>" << endl;
	file << "<DataArray NumberOfComponents=\"3\" type=\"Float32\" format=\"ascii\" >" << endl;
	for (int n=0;n<node_count;++n) {
		for (int i=0; i<3; ++i) 
			file<< setw(16) << setprecision(8) << scientific << node[3*n+i] << endl;
	}
	file << "</DataArray>" << endl;
	file << "</Points>" << endl;
	file << "<Cells>" << endl;

	file << "<DataArray Name=\"connectivity\" type=\"Int32\" format=\"ascii\" >" << endl;
	for (int c=0;c<elem_count;++c) {
		for (int n=0;n<3;++n) {
			file << elcon[3*c+n] << "\t";
		}
		file << endl;
	}

	file << "</DataArray>" << endl;
	file << "<DataArray Name=\"offsets\" type=\"Int32\" format=\"ascii\" >" << endl;
	int offset=0;
	for (int c=0;c<elem_count;++c) {
		offset+=3;
		file << offset << endl;
	}
	file << "</DataArray>" << endl;

	file << "<DataArray Name=\"types\" type=\"UInt8\" format=\"ascii\" >" << endl;
	for (int c=0;c<elem_count;++c) {
		file << "5" << endl; // Tetra //Code is "5 " OR "VTK_TRIANGLE"
		// if (Cell(c).Num_Vertex()==4) file << "10" << endl; // Tetra
		// if (Cell(c).Num_Vertex()==8) file << "12" << endl; // Hexa
		// if (Cell(c).Num_Vertex()==6) file << "13" << endl; // Prism
		// if (Cell(c).Num_Vertex()==5) file << "14" << endl; // Pyramid (Wedge)
	}
	file << endl;
	file << "</DataArray>" << endl;;

	file << "</Cells>" << endl;

	file << "<PointData Scalars=\"scalars\" format=\"ascii\">" << endl;


	file << "</PointData>" << endl;

	file << "</Piece>" << endl;
	file << "</UnstructuredGrid>" << endl;
	file << "</VTKFile>" << endl;
	file.close();

	return;
}

NastranReader::~NastranReader(){
  
  delete [] node;
  delete [] elcon;
  delete [] nodeid;
}
