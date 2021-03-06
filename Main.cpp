// http://globalplm.com/microsoft-visual-studio-project-configuration-for-teamcenter-unified-itk/

#include "Common/Utilities.h"
#include <typeinfo>
#include <regex>

#include "Common/FileParser.h"
#include "Common/wi_item.h"
#include "Common/wi_dataset.h"
#include <iostream>
#include <fstream>
#include <chrono>
#include <locale> 

#include <string>
#include <map>
#include <sys/stat.h>
#include <windows.h>
#include <stdlib.h>
#include <me/meline.h>
#include <bom/bom.h>
#include <cfm/cfm.h>
#include <me/me.h>
#include <ps/ps.h>
#include <pie/pie.h>
#include <tccore/aom.h>
#include <tccore/item.h>
#include <tccore/tctype.h>
#include <tccore/grm.h>
#include <conio.h>
#include <fclasses/tc_date.h>
#include <ps/absocc.h>


#include <tccore/project.h>
#include <tccore/aom_prop.h>
#include <tccoreext/gdetype.h>
#include <tccore/releasestatus.h>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>

#include "Common/wi_errors.h"

#include "XMLParser.h"
#include <pom\enq\enq.h>
#include <sa\user.h>
#include <epm/epm.h>
#include <lov\lov.h>

#include <direct.h>


#include <ae/ae.h>
#include <sa\tcvolume.h>
#include <tc/tc.h>

#include <stdlib.h>
#include <tc/tc.h>
#include <sa/tcfile.h>
#include <tccore/workspaceobject.h>
#include <ae/ae.h>
#include <user_exits/user_exits.h>
#include <property/prop.h>
#include <sa/tcvolume.h>
#include <tccore/aom.h>
#include <ss/ss_const.h>
#include <stdarg.h>
#include <sa\tcfile_cache.h>

using boost::property_tree::ptree;
using boost::property_tree::read_json;
using boost::property_tree::write_json;
using boost::property_tree::read_xml;
using namespace std;

string fileName = "";
string path_g = "";

const char* months[12] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul",
		"Aug", "Sep", "Oct", "Nov", "Dec" };

logical dispatch;

bool createDir(string path) {
	struct stat info;
	bool status = false;
	if (stat(path.c_str(), &info) != 0) {
		int result = mkdir(path.c_str());
		if (result) {
			//error
			ofstream myfile;
			myfile.open(fileName, ios_base::app);
			myfile << "ERROR: Unable to create dirs" << std::endl;

		}
		else {
			status = true;
		}
	}

	return status;
}

void prepDirs(string id, string rev_id) {
	std::string path = "//wimanufacturing/Test/test_ds/documents/" + id;

	//create new dir for PP if it does not exist already
	bool status = createDir(path);
	if (status == false) {
		exit(1);
	}
	//create dir for PP Rev
	path = path + "/" + rev_id;
	status = createDir(path);
	if (status == false) {
		exit(1);
	}

	path_g = path;

	return;

}

string get_current_dir() {
	char* buffer;

	// Get the current working directory:
	if ((buffer = _getcwd(NULL, 0)) == NULL)
		perror("_getcwd error");
	else
	{
		printf("%s \nLength: %zu\n", buffer, strlen(buffer));

		return buffer;

		free(buffer);
	}
}

void setProperty(tag_t item_tag, std::string name, string value) {
	char 
		*value_changed,
		*item_type;
	tag_t item_type_t = NULLTAG;
	int ifail = 0;

	WSOM_ask_object_type2(item_tag, &item_type);
	TCTYPE_ask_type(item_type, &item_type_t);

	
	TERR(AOM_lock(item_tag));

	ifail = AOM_UIF_set_value(item_tag, name.c_str(), value.c_str());

	TERR(AOM_save_without_extensions(item_tag));

	AOM_refresh(item_tag, 0);

	
	if (ifail) {
		ofstream myfile;
		myfile.open(fileName, ios_base::app);
		myfile << "WARNING: Unable to set property " << name << " with value of " << value << " on object of type " << item_type << std::endl;

	}
		
	

}

void setProperty(tag_t item_tag, std::string name, tag_t value) {
	try {
		TERR(AOM_lock(item_tag));

		TERR(AOM_set_value_tag(item_tag, name.c_str(), value));

		TERR(AOM_save_without_extensions(item_tag));
	}
	catch (ITKError& e) {
		cout << nLOC << e.what();
	}

}

std::vector<std::string> split_string(std::string word, char delim) {
	std::stringstream ss(word);
	std::string segment;
	std::vector<std::string> seglist;

	while (std::getline(ss, segment, delim))
	{
		//segment.erase(std::remove(segment.begin(), segment.end(), '\n'), segment.end()); //remove newline from segments
		seglist.push_back(segment);
	}
	return seglist;

}

tag_t findUser(const char * username)
{

	void
		*** report;
	int
		row = 0,
		n_rows = 0,
		n_cols = 0,
		status_int;
	const char
		* select_attr_list[] = { "person" };
	char
		* puid_string = NULL,
		status_string[10] = "";
	char* query = "findUser";
	/*create a query*/

	tag_t user_t = NULLTAG;

	try {

		TERR(POM_enquiry_create(query));
		TERR(POM_enquiry_set_distinct(query, true));

		/*add the list to the select clause of the query*/
		TERR(POM_enquiry_add_select_attrs(query, "User", 1, select_attr_list));

		TERR(POM_enquiry_set_string_value(query, "name", 1, &username, POM_enquiry_const_value));
		TERR(POM_enquiry_set_attr_expr(query, "where_expr", "User", "user_name", POM_enquiry_equal, "name"));
		TERR(POM_enquiry_set_where_expr(query, "where_expr"));


		TERR(POM_enquiry_execute(query, &n_rows, &n_cols, &report));

	}
	catch (ITKError e) {
		cout << nLOC << e.what();
	}

	cout << nLOC << "Number of Rows: " << n_rows;
	row = n_rows - 1;
	if (row == 0)
	{
		user_t = *((tag_t*)report[row][0]);
	}
	MEM_free(report);

	POM_enquiry_delete(query);
	return user_t;
}

tag_t getTimeAnalysisForm(tag_t oper) {
	int
		n_attach,
		jj;
	GRM_relation_t
		* attachs;

	tag_t form = NULLTAG;

	GRM_list_secondary_objects(oper, NULL, &n_attach, &attachs);
	//returns a list of relations in pairs (primary, secondry), where the WSO that called it could be either primary or secondary. 

	for (jj = 0; jj < n_attach; jj++)
	{
		char* name;

		TCTYPE_ask_name2(attachs[jj].relation_type, &name);

		if (strcmp(name, "METimeAnalysisRelation") == 0) {
			form = attachs[jj].secondary;
		}
	}

	MEM_free(attachs);

	return form;
}

void setTimeFormProps(tag_t oper, string CycleBatch, string LaborRun, string LaborSetup, string MachineRun, string MachineSetup, string Transport) {
	tag_t form = getTimeAnalysisForm(oper);
	setProperty(form, "w4_CycleBatch", CycleBatch);
	setProperty(form, "w4_LaborRun", LaborRun);
	setProperty(form, "w4_LaborSetup", LaborSetup);
	setProperty(form, "w4_MachineRun", MachineRun);
	setProperty(form, "w4_MachineSetup", MachineSetup);
	setProperty(form, "w4_Transport", Transport);
}

tag_t get_persons_user_tags(const char* name)
{
	int
		n_all_users = 0,
		count = 0;
	tag_t
		* all_user_tags = NULL,
		user_tag = NULL;
	char
		* person_name;

	SA_extent_user(&n_all_users, &all_user_tags);
	for (int ii = 0; ii < n_all_users; ii++)
	{

		SA_ask_user_person_name2(all_user_tags[ii], &person_name);
		if (strcmp(person_name, name) == 0)
		{
			user_tag = all_user_tags[ii];
			break;

		}
	}
	MEM_free(all_user_tags);
	return user_tag;
}

static void modify_date_created(tag_t item, string date_creation)
{
	date_t
		set_date;

	printf("\nChanging Creation Date..");

	char *date_string1;

	//format date first
	//1/7/2009  7:50:28 AM -> 07-Jan-2009 07:50
	vector<string> full_date = split_string(date_creation, ' ');
	vector<string> date_pt1 = split_string(full_date[0], '/'); // 1/7/2009
	string day = date_pt1[1];
	int month = stoi(date_pt1[0]);
	string month_str = months[month - 1];
	string year = date_pt1[2];
	string format_date = day + "-" + month_str + "-" + year;
	ITK_ask_default_date_format(&date_string1);


	TERR(AOM_refresh(item, TRUE));
	TERR(ITK_string_to_date(format_date.c_str(), &set_date));
	TERR(POM_set_env_info(POM_bypass_attr_update, FALSE, 0, 0, NULLTAG, NULL));
	TERR(POM_set_creation_date(item, set_date));

	TERR(AOM_save_without_extensions(item));
}

static void modify_date_released(tag_t item, string date_released)
{
	date_t
		set_date;
	tag_t attr_tag = NULLTAG;

	printf("\nChanging Released Date..");

	char* date_string1;

	//format date first
	//1/7/2009  7:50:28 AM -> 07-Jan-2009 07:50
	vector<string> full_date = split_string(date_released, ' ');
	vector<string> date_pt1 = split_string(full_date[0], '/'); // 1/7/2009
	string day = date_pt1[1];
	int month = stoi(date_pt1[0]);
	string month_str = months[month - 1];
	string year = date_pt1[2];
	string format_date = day + "-" + month_str + "-" + year;
	ITK_ask_default_date_format(&date_string1);


	TERR(AOM_refresh(item, TRUE));
	TERR(ITK_string_to_date(format_date.c_str(), &set_date));
	TERR(POM_set_env_info(POM_bypass_attr_update, FALSE, 0, 0, NULLTAG, NULL));
	
	IFERR_REPORT(POM_attr_id_of_attr("date_released", "Item", &attr_tag));
	IFERR_REPORT(POM_set_attr_date(1, &item, attr_tag, set_date));

	TERR(AOM_save_without_extensions(item));

}

static void verifyPartAttrs(tag_t part, string wi_team, logical serialized, logical serialTransfer) {
	char 
		*value,
		*object_name;	int
		n_attach,
		jj;
	tag_t
		* attachs,
		object_type,
		partAttrForm = NULLTAG;
	logical 
		value_l;
	std::vector<tag_t> items(NULLTAG);

	ofstream myfile;
	myfile.open(fileName, ios_base::app);


	TERR(GRM_list_secondary_objects_only(part, NULLTAG, &n_attach, &attachs));
	for (int ii = 0; ii < n_attach; ++ii) {
		TCTYPE_ask_object_type(attachs[ii], &object_type);
		TCTYPE_ask_name2(object_type, &object_name);
		if (strcmp(object_name, "W4_PartAttrForm") == 0) {
			partAttrForm = attachs[ii];
			break;
		}
	}
	
	if (partAttrForm != NULLTAG) {
		//get cell code from dynamic LOV
		AOM_UIF_ask_value(partAttrForm, "w4_ResponsibleCell", &value);

		wi_item w4_cell(value, "A");

		w4_cell.getPN_t();

		char* wso_name;
		WSOM_ask_name2(w4_cell.getPN_t(), &wso_name);

		if (strcmp(wi_team.c_str(), wso_name) != 0) {
			myfile << "WARNING: " << "WI_TEAM (" << wi_team << ") does not equal Responsible Cell (" << value << ":" << wso_name << ") on Part Attr Form." << endl;
		}
		//serialized
		TERR(AOM_ask_value_logical(partAttrForm, "w4_Serialized", &value_l));
		if (serialized != value_l) {
			myfile << "WARNING: " << "WPLAN_SNREQ (" << serialized << ") does not equal Serialized (" << value_l << ") on Part Attr Form." << endl;
		}
		//serial transfer
		TERR(AOM_ask_value_logical(partAttrForm, "w4_SerialTransfer", &value_l));
		if (serialTransfer != value_l) {
			myfile << "WARNING: " << "WPLAN_SNTRANS (" << serialTransfer << ") does not equal Serial Transfer (" << value_l << ") on Part Attr Form." << endl;
		}
	}
	else {
		myfile << "No Part Attr Form Found on this part" << endl;
	}

	myfile.close();
	
}

//the material desc attributes are rarely ever going to match exactly...so i never finished this function.
static void verifyMaterialAttrs(wi_item material) {
	char* desc = NULL;
	AOM_UIF_ask_value(material.getPN_t(), "object_desc", &desc);

	std::vector<std::string> mat_desc_vec = split_string(desc, ';');
	//compare mat values from json to mat desc values.

}

static void setReleaseStatus(tag_t item, const char* status) {

	tag_t status_t = NULLTAG;
	int n_statuses = 0;
	tag_t* statuses = NULLTAG;
	char
		* name = NULL;

	wi_item item_dummy("043535", "A");

	WSOM_ask_release_status_list(item_dummy.getPN_t(), &n_statuses, &statuses);

	for (int ii = 0; ii < n_statuses; ii++)
	{
		TERR(AOM_ask_name(statuses[ii], &name));
		if (strcmp(name, status) == 0) {
			tag_t items[] = { item };
			//TERR(EPM_find_status_type(status, &status_t));
			TERR(AOM_refresh(item, true));
			TERR(RELSTAT_add_release_status(statuses[ii], 1, items, true));
			TERR(AOM_refresh(item, true));
			TERR(AOM_save_without_extensions(item));
		}
	}
	if (name) MEM_free(name);
	if (n_statuses) MEM_free(statuses);
	
	
}

void setOperProps(tag_t oper_rev, string W1ST_PC_INSP, string WLOT_REQD, string WSPR_REQD, string WRUN_REQD, string WRFA_REQD) {
	string
		W1ST_PC_INSP_Log,
		WLOT_REQD_Log,
		WSPR_REQD_Log,
		WRUN_REQD_Log,
		WRFA_REQD_log;

	if (W1ST_PC_INSP == "NO") {
		W1ST_PC_INSP_Log = "false";
	}
	else if (W1ST_PC_INSP == "YES") {
		W1ST_PC_INSP_Log = "true";
	}
	if (WLOT_REQD == "NO") {
		WLOT_REQD_Log = "false";
	}
	else if (WLOT_REQD == "YES") {
		WLOT_REQD_Log = "true";
	}
	if (WSPR_REQD == "NO") {
		WSPR_REQD_Log = "false";
	}
	else if (WSPR_REQD == "YES") {
		WSPR_REQD_Log = "true";
	}
	if (WRUN_REQD == "NO") {
		WRUN_REQD_Log = "false";
	}
	else if (WRUN_REQD == "YES") {
		WRUN_REQD_Log = "true";
	}
	if (WRFA_REQD == "NO") {
		WRFA_REQD_log = "false";
	}
	else if (WRFA_REQD == "YES") {
		WRFA_REQD_log = "true";
	}

	setProperty(oper_rev, "w4_FirstPieceReq", W1ST_PC_INSP_Log);
	setProperty(oper_rev, "w4_LotReq", WLOT_REQD_Log);
	setProperty(oper_rev, "w4_SPCDataReq", WSPR_REQD_Log);
	setProperty(oper_rev, "w4_FurnaceRunNoReq", WRUN_REQD_Log);
	setProperty(oper_rev, "w4_RFAReq", WRFA_REQD_log);


}

void write_plmxml_file(tag_t bvr_tag, string path)
{
	cout << nLOC << path;
	cout << nLOC << "Writing PLMXML." << flush;

	tag_t window = NULLTAG;

	try {
		TERR(ME_create_bop_window(&window));

		tag_t rule = NULLTAG;
		TERR(CFM_find("Latest Working", &rule));

		TERR(BOM_set_window_config_rule(window, rule));
		TERR(BOM_set_window_pack_all(window, false));  // set to false as default

		tag_t top_line = NULLTAG;
		//TERR(BOM_set_window_top_line(window, NULLTAG, item_rev, NULLTAG, &top_line));
		TERR(BOM_set_window_top_line_bvr(window, bvr_tag, &top_line));

		int n_selected = 0;
		tag_t* selected = NULL;
		TERR(BOM_line_ask_child_lines(top_line, &n_selected, &selected));

		BOM_writer_output* output = 0;
		TERR(BOM_writer_new_output_file(&output));
		output->file.filehandle = fopen(path.c_str(), "r");
		output->common.object = MEM_string_copy(path.c_str());

		BOM_writer_format* format = 0;
		TERR(BOM_writer_new_format_plmxml(&format));
		format->plmxml.builder_name = MEM_string_copy("AbsoluteOccurrences");
		format->plmxml.transform_type = TransformType_AbsOcc;

		format->plmxml.transfer_mode = MEM_string_copy("_sl_MESExport");
		//format->plmxml.transfer_mode = MEM_string_copy("ConfiguredDataFilesExportDefault");   

		BOM_writer_traversal* traversal = 0;
		TERR(BOM_writer_new_traversal(&traversal));
		traversal->selected_count = 1;
		traversal->selected_lines = &top_line;
		traversal->no_descendants = false;
		traversal->transient_unpack = true;

		cout << nLOC << "XML Exporting...";
		TERR(BOM_writer_write_bomwindow(window, output, format, traversal));
		MEM_free(selected);

		cout << nLOC << "XML Export Complete.";

	}
	catch (ITKError& e) {
		cout << nLOC << e.what();
	}

	TERR(BOM_close_window(window));
	cout << nLOC << "XML Export Complete.";
}

void createWorkInsts(string s)
{
	remove("\\temp\\temp.txt");
	ofstream myfile;
	myfile.open("\\temp\\temp.txt");
	myfile << s;
	myfile.close();
	system("\"C:\\Program Files (x86)\\Microsoft Office\\Office14\\powerpnt.exe\" /m \\temp\\plans\\tools\\ppt-template.pptm convertc");
}

void exportPDF(wi_dataset pdf, string path) {
	int ifail = 0;
	ofstream myfile;
	tag_t
		* refernced_object = NULLTAG,
		single_ref;
	int
		nFound = 0;

	AE_ask_all_dataset_named_refs2(pdf.getDS_t(), "PDF_Reference", &nFound, &refernced_object);

	
	if (nFound > 0) {

		single_ref = refernced_object[0]; //do not know if this gets most recent or latest....or neither..?

		MEM_free(refernced_object);
	}

	
	string local_path = get_current_dir() + "/" + pdf.getObjectName() + ".pdf";

	ifail = IMF_export_file(single_ref, local_path.c_str());
	if (ifail) {
		std::string error = "ERROR: Failed to export dataset " + pdf.getDS_t();
		myfile.open(fileName, ios_base::app);
		string op_lbl = pdf.getObjectName();
		myfile << "ERROR: Failed to export dataset " + op_lbl << endl;
		myfile.close();

	}
	else {
		if (std::rename(local_path.c_str(), path.c_str()) < 0) {
			std::cout << strerror(errno) << '\n';
		}
	}
}

void deletePPTX(wi_item& myStp) {
	int count;
	GRM_relation_t
		* attachs;

	GRM_list_all_related_objects(myStp.getREV_t(), &count, &attachs);
	for (int ii = 0; ii < count; ++ii) {
		tag_t relation_type = REL_winst();
		if (attachs[ii].relation_type == relation_type) {
			//found
			GRM_delete_relation(attachs[ii].the_relation);
		}
	}
}

void boostWorkInstructions(wi_item& myStp, int objnbr, string op_id, ptree& pt)
{
	string ppt = "";

	BOOST_FOREACH(boost::property_tree::ptree::value_type & v, pt.get_child("plan.STEXTs.")) {
		if (v.second.get<int>("OP_NBR") == objnbr) {
			string txtstr = "\n\f" + v.second.get<string>("MFG_TEXT");
			ppt.append(txtstr);

			cout << "\n\tTEXT: " << op_id << "_" << v.second.get<string>("STP_SEQ");
		}
	}

	string imgloc = "\\\\ogserver4\\cimx\\pictures\\";
	BOOST_FOREACH(boost::property_tree::ptree::value_type & v, pt.get_child("plan.SSKETs.")) {
		if (v.second.get<int>("OP_NBR") == objnbr) {
			string imagestr = "\n\f<" + imgloc + v.second.get<string>("CSCAPP_FILE") + ">";
			ppt.append(imagestr);

			cout << "\n\tImage: " << v.second.get<string>("CSCAPP_FILE");
		}
	}


	if (ppt.length() > 0) {

		createWorkInsts(ppt);

		//string fNamePPTX = "\\temp\\"+ op_id + ".pptx";
		//string fNamePDF = "\\temp\\" + op_id + ".pdf";

		string fNamePPTX = "\\temp\\work_instructions.pptx";
		string fNamePDF = "\\temp\\work_instructions.pdf";
		

		//need to delete the pptx file made on create so there isnt two. EDIT
		deletePPTX(myStp);

		myStp.linkWorkInstructions(wi_dataset(op_id.c_str(), "MSPowerPointX", "PPTX", fNamePPTX.c_str()));
		wi_dataset ds(op_id.c_str(), "PDF", "PDF", fNamePDF.c_str());

		myStp.linkWorkInstructions(ds);
		string path = path_g + "/" + op_id + ".pdf";

		exportPDF(ds, path);
	}

	cout.flush();
}



void boostTools(wi_item& myStp, int objnbr, ptree& pt)
{
	cout << nLOC << "starting Tools";
	if (pt.empty()) return;

	auto val = pt.get_child_optional("plan.STOOLs.");

	if (val) {
		BOOST_FOREACH(boost::property_tree::ptree::value_type & v, pt.get_child("plan.STOOLs.")) {
			if (v.second.get<int>("OP_NBR") == objnbr) {
				//cout << "\n\tTool: " << v.second.get<string>("TYPE") << v.second.get<string>("TOOL_NUMBER");
				string tool_type = v.second.get<string>("TYPE");
				string tool_number = v.second.get<string>("TOOL_NUMBER");
				//check for 6 digits
				if (tool_number.size() > 6) {
					for (int ii = tool_number.size(); ii <= 6; ++ii) {
						string zero = "0";
						tool_number = zero + tool_number;
					}
				}

				string temp_tool_id = tool_type + tool_number;
				string tool_desc = v.second.get<string>("TOOL_DESC");
				int tool_qty = v.second.get<int>("NUMBER_REQD");
				try
				{
					cout << "\n Adding tool: " << temp_tool_id;
					wi_item tool(temp_tool_id, "A");
					if (!tool.getPN_t()) {
						wi_item t2;
						/*
						if (strcmp(tool_type.c_str(), "G") == 0) {
							t2.instantiate(get_item_type("W4_GTool"), get_item_type("W4_GToolRevision"), temp_tool_id, "A", tool_desc, tool_desc, true);

						}
						else if (strcmp(tool_type.c_str(), "TL") == 0) {
							t2.instantiate(get_item_type("W4_TLTool"), get_item_type("W4_TLToolRevision"), temp_tool_id, "A", tool_desc, tool_desc, true);

						}
						else if (strcmp(tool_type.c_str(), "TL") == 0) {
							t2.instantiate(get_item_type("W4_Tooling"), get_item_type("W4_ToolingRevision"), temp_tool_id, "A", tool_desc, tool_desc, true);
						}
						else {

						}
						myStp.addTool(t2, tool_qty);
						*/
						//if tool DNE, then flag it and make uiser create the tool.
						ofstream myfile;
						myfile.open(fileName, ios_base::app);
						myfile << "WARNING: The Tool " << temp_tool_id << " does not exist and needs to be created under OP_NBR=" << objnbr << endl;


					}
					else {
						myStp.addTool(tool, tool_qty);
					}
				}
				catch (exception& e) {
					cout << e.what();
				}

			}
			cout.flush();
		}
	}
	else {
		cout << nLOC << "No TOOLs";
	}


}

void boostTime(wi_item& myStp, double WMACH_CYC, double WMACH_SU)
{
	try
	{
		tag_t opForm = getSingleForm(myStp.getPN_t(), REL_ref(), "W4_MEOPForm");
		if (!opForm) {
			create_simple_form(get_item_type("W4_MEOPForm"), "CIMX_Time Form", "", &opForm);
			tag_t myRel = NULLTAG;
			create_simple_relation(myStp.getREV_t(), opForm, REL_ref(), &myRel);
		}

		if (opForm) {
			int ifail = (AOM_refresh(opForm, TRUE)); // Lock, Load, Change
			if (!ifail) ifail = AOM_set_value_double(opForm, "w4_StdRunTime", WMACH_CYC);
			if (!ifail) ifail = AOM_set_value_double(opForm, "w4_StdSetupTime", WMACH_SU);
			if (!ifail) ifail = AOM_save_with_extensions(opForm);
			cout << "\n **** Success: Time Form!";

		}
		else {
			cout << "\n **** Time Form Not found or Created!";
		}
	}
	catch (exception& e) {
		cout << e.what();
	}

	cout.flush();
}

void boostWorkCenter(wi_item& myOp, string name, string desc)
{
	// Configure Operation
	wi_item wc(name, "A");
	if (!wc.getREV_t()) {
		wi_item wc2;
		cout << "\nCreating:" << wc.getPN();
		wc2.instantiate(get_item_type("W4_WorkCenter"), get_item_type("W4_WorkCenterRevision"), name, "A", desc, "", true);
		myOp.addWorkcenter(wc2);
	}
	else {
		cout << "\nFound:" << wc.getPN();
		myOp.addWorkcenter(wi_item(name, "A"));
	}
	cout.flush();
}

void boostSTD(wi_item& myStp, int objnbr, ptree& pt)
{
	/*
		BOOST_FOREACH(boost::property_tree::ptree::value_type &v, pt.get_child("plan.OSTDIs.")) {
			if (v.second.get<int>("OP_NBR")==objnbr) {
				//cout << "\n\tSTDI: " << v.second.get<string>("INST_NAME") << " - " << v.second.get<string>("INST_TYPE") << ": " << v.second.get<string>("STD_INSTRUCTION");
			}
		}
	*/
	cout.flush();
}

void boostMaterial(wi_item& myStp, string material)
{
	ofstream myfile;

	try
	{
		cout << nLOC << "starting Material";
		tag_t mat_tag;
		ITEM_find_item(material.c_str(), &mat_tag); //this could be a raw mat...item...mfg...
		if (mat_tag == NULLTAG) {
			//mat does not exist
			myfile.open(fileName, ios_base::app);
			myfile << "WARNING: Material " << material << " does not exist in PV." << endl;
			myfile.close();	
		}
		else {
			wi_item material_item(mat_tag); //assuming this is always NA rev....
			myStp.addMaterial(material_item);

		}
				
	}
	catch (exception& e) {
		cout << e.what();
	}
}

void boostChars(wi_item& myStp, int objnbr, ptree& pt)
{
	cout << nLOC << "starting Chars";
	if (pt.empty()) return;
	tag_t gdeBvr = NULLTAG;

	auto val = pt.get_child_optional("plan.SCHARs.");

	if (val) { //check to make sure CHARs child node exists
		BOOST_FOREACH(boost::property_tree::ptree::value_type & v, pt.get_child("plan.SCHARs.")) {
			if (v.second.get<int>("OP_NBR") == objnbr) {

				if (!gdeBvr) gdeBvr = myStp.getGdeBvr_t();  // force creation of gdebvr if needed.

				string chr_lbl = "CHAR_" + v.second.get<string>("CHR_LBL");
				string chr_desc = v.second.get<string>("CHAR_DESC");
				string chr_freq = v.second.get<string>("WFREQUENCY");
				string chr_tool = v.second.get<string>("TOOL_GAGE_NO");
				string chr_zone = v.second.get<string>("NOTE_ZONE");
				string chr_class = v.second.get<string>("CHAR_CLASS");

				if (chr_desc.length() > 0) myStp.createDCD(chr_lbl, chr_desc, chr_freq, chr_tool, chr_zone, chr_class);
				cout.flush();
			}
		}
	}
	else {
		cout << nLOC << "No TOOLs";
	}
}

void boostOperations(wi_item& myPrc, ptree& pt)
{
	wi_item prevOp; //NO LONGER DOING SUB STUFF.
	// Iterate overall of the operations!
	int oper_count = 1;
	BOOST_FOREACH(boost::property_tree::ptree::value_type & p, pt.get_child("plan.OHEADs.")) {
		int objnbr = p.second.get<int>("OP_NBR");
		string op_id = p.second.get<string>("OP_LBL");
		string op_type = p.second.get<string>("OPER_TYPE");

		cout << "\n **** Plan: " << p.second.get<string>("OP_LBL") << " - " << p.second.get<string>("OPER_TYPE");

		/*************** Operation Group **************/
		cout << nLOC << "starting OpGroup";
		wi_item myOp = wi_item();
		if (dispatch)
		{
			cout << nLOC << "Dispatch Process";
			myOp.instantiate(get_item_type("MEProcess"), get_item_type("MEProcessRevision"), "", "000", p.second.get<string>("OP_LBL"), p.second.get<string>("OPER_DESC"), true);
		}
		else
		{
			cout << nLOC << "Process";
			myOp.instantiate(get_item_type("W4_Process"), get_item_type("W4_ProcessRevision"), "", "000", p.second.get<string>("OP_LBL"), p.second.get<string>("OPER_DESC"), true);
		}
		//may run into problem when first op in occ list is the ALTERNATE. 
		//Currently we are assuming that doesnt happen...
		//NO LONGER DOING THIS!!!! SAVE ANYWAY.
		/*
		if (op_type == "PRIMARY") {
			myPrc.addOp(myOp);
			prevOp = myOp;
		}
		else if (op_type == "ALTERNATE") {
			myPrc.addOpSub(myOp, prevOp.getOCC());
			//keep prevOp as it is since the current Op is a substitute
		}
		*/
		myPrc.addOp(myOp);


		boostWorkCenter(myOp, p.second.get<string>("OPER_WORK_CENTER"), p.second.get<string>("WMACH_DESC"));

		/*************** Operation ********************/
		cout << nLOC << "starting Operation";
		wi_item myStp = wi_item();
		if (dispatch)
		{
			cout << nLOC << "Dispatch Operation";
			myStp.instantiate(get_item_type("W4_Operation"), get_item_type("W4_OperationRevision"), "", "000", p.second.get<string>("OP_LBL"), p.second.get<string>("OPER_DESC"), true);
		}
		else
		{
			cout << nLOC << "ManualWork";
			myStp.instantiate(get_item_type("W4_ManualWork"), get_item_type("W4_ManualWorkRevision"), "", "000", p.second.get<string>("OP_LBL"), p.second.get<string>("OPER_DESC"), true);
		}

		myOp.addOp(myStp);

			cout << nLOC << "New OpStp:" << myStp.getPN() << "/" << myStp.getREV();

		//boostTime(myStp, p.second.get("WMACH_CYC", 0.f), p.second.get("WMACH_SU", 0.f));

		setTimeFormProps(myOp.getREV_t(), p.second.get<string>("WCYCLE_BATCH"), p.second.get<string>("WLABOR_CYC"), p.second.get<string>("WLABOR_SU"), p.second.get<string>("WMACH_CYC"), p.second.get<string>("WMACH_SU"), p.second.get<string>("WTRANS_HR"));

		//operation props
		string W1ST_PC_INSP = p.second.get<string>("W1ST_PC_INSP");
		string WLOT_REQD = p.second.get<string>("WLOT_REQD");
		string WSPR_REQD = p.second.get<string>("WSPR_REQD");
		string WRUN_REQD = p.second.get<string>("WRUN_REQD");
		string WRFA_REQD = p.second.get<string>("WRFA_REQD");

		setOperProps(myOp.getREV_t(), W1ST_PC_INSP, WLOT_REQD, WSPR_REQD, WRUN_REQD, WRFA_REQD);


		//boostSTD(myStp, objnbr, pt);
		boostWorkInstructions(myStp, objnbr, op_id, pt);
		boostTools(myStp, objnbr, pt);
		boostChars(myStp, objnbr, pt);

		string material = pt.get<string>("plan.PPLANs..WPLAN_MATL");
		if (oper_count == 1) {
			//only the first oper step rev should have the raw material on it.
			boostMaterial(myStp, material);

		}

		cout.flush();

		oper_count++;

	}
}
/*
void doTest1() {
	wi_item testPart("OP000995", "000");
	testPart.instantiate("W4_Operation", "My Operation", "Automated", true);
	cout << nLOC << "Starting";
	testPart.loadAll();
	testPart.createDCD("ProgramTest997", "Test", "f", "t", "z", "c");
	testPart.createDCD("ProgramTest996", "Test", "f", "t", "z", "c");
}

void doTest2() {
	wi_item testPart("OP000953", "000");
	cout << nLOC << "Starting";
	testPart.loadAll();
	testPart.createDCD("ProgramTest999", "Test", "f", "t", "z", "c");
}

void doTest3() {
	cout << nLOC << "Starting";
	wi_item myPrc("PRC221621_800_Prod", "A");
	myPrc.loadAll();

	cout << nLOC << "Paths";
	string xml_path = "\\temp\\plans\\xml\\" + myPrc.getPN() + ".xml";
	string json_mpp_path = "\\temp\\plans\\xml\\" + myPrc.getPN() + ".json";

	cout << nLOC << "Exporting";
	write_plmxml_file(myPrc.getBVR_t(), xml_path);

	cout << nLOC << "Parsing";
	XMLParser xml;

	cout << nLOC << "Indexing";
	xml.InitIndex(xml_path);

	cout << nLOC << "Writing Json";
	xml.toJSON(json_mpp_path);

}



void doTest4() {
	cout << nLOC << "Starting";
	wi_item myPrc("PRC221621_800_Prod", "A");
	myPrc.loadAll();

	cout << nLOC << "Paths";
	string xml_path = "\\temp\\plans\\xml\\" + myPrc.getPN() + ".xml";
	string json_mpp_path = "\\temp\\plans\\xml\\" + myPrc.getPN() + ".json";

	cout << nLOC << "Exporting";
	write_plmxml_file(myPrc.getBVR_t(), xml_path);

	cout << nLOC << "Translating";
	string cmd = "\\temp\\plans\\tools\\xsl2json2.js \\temp\\plans\\tools\\" + myPrc.getPN() + " t7";
	system(cmd.c_str());
}

void doTest5() {
	cout << nLOC << "Starting";
	wi_item myPrc("PRC221621_800_Prod", "A");
	myPrc.loadAll();

	cout << nLOC << "Paths";
	string xml_path = "\\temp\\plans\\xml\\" + myPrc.getPN() + ".xml";
	string json_mpp_path = "\\temp\\plans\\xml\\" + myPrc.getPN() + ".json";

	cout << nLOC << "Translating";
	string cmd = "\\temp\\plans\\tools\\xsl2json2.js \\temp\\plans\\tools\\" + myPrc.getPN() + " t7";
	system(cmd.c_str());
}
*/

void doPlan(string target_plan) {
	ofstream myfile;

	cout << nLOC << "Starting plan: " << target_plan;

	string json_cimx_path = "\\temp\\plans\\xml\\" + target_plan + ".txt";
	cout << nLOC << "Target: " << json_cimx_path;

	GetFileAttributes(json_cimx_path.c_str()); // from winbase.h
	if (INVALID_FILE_ATTRIBUTES == GetFileAttributes(json_cimx_path.c_str()) && GetLastError() == ERROR_FILE_NOT_FOUND)
	{
		cout << nLOC << "File " << json_cimx_path << " not found." << endl;
	}

	ptree pt;
	try {
		read_json(json_cimx_path, pt);
		cout << nLOC << json_cimx_path << " Read Successfully!";
	}
	catch (exception& e)
	{
		myfile.open("Error_Generic.log", ios_base::app);
		myfile << nLOC << "Error Reading: " << json_cimx_path << flush;
		myfile << nLOC << "Check reported line number for issues with JSON file." << endl;
		myfile << nLOC << e.what() << flush;
		myfile.close();
		exit(0);
		
	}

	string part_id;
	string part_revision;
	string wplan_code;
	string wplan_rev;
	string plan_desc;
	string wplan_crit_sens;
	string plan_prod_stat;
	string plan_creator;
	string plan_rev_author;
	string plan_create_date;
	string plan_rev_date;
	string plan_edit_date;
	string wplan_serial_num_req;
	string wplan_serial_num_trans;
	string wplan_mat;
	string wi_team;

	try {
		
		part_id = pt.get<string>("plan.PPLANs..PART_ID");
		part_revision = pt.get<string>("plan.PPLANs..PART_REVISION");
		wplan_code = pt.get<string>("plan.PPLANs..WPLAN_CODE");
		wplan_rev = pt.get<string>("plan.PPLANs..WPLAN_REV");
		plan_desc = pt.get<string>("plan.PPLANs..PLANDESC");
		wplan_crit_sens = pt.get<string>("plan.PPLANs..WPLAN_CRIT_SEN");
		plan_prod_stat = pt.get<string>("plan.PPLANs..PLAN_PRODSTAT");
		plan_creator = pt.get<string>("plan.PPLANs..PLAN_CREATOR");
		plan_rev_author = pt.get<string>("plan.PPLANs..PLAN_REV_AUTHOR");
		plan_create_date = pt.get<string>("plan.PPLANs..PLAN_CREATE_DATE");
		plan_rev_date = pt.get<string>("plan.PPLANs..PLAN_REV_DATE");
		plan_edit_date = pt.get<string>("plan.PPLANs..LAST_EDIT_DATE"); //DNW

		wplan_serial_num_req = pt.get<string>("plan.PPLANs..WPLAN_SNREQ");
		wplan_serial_num_trans = pt.get<string>("plan.PPLANs..WPLAN_SNTRANS");

		wplan_mat = pt.get<string>("plan.PPLANs..WPLAN_MATL");
		wi_team = pt.get<string>("plan.PPLANs..WI_TEAM");
		
	}
	catch (exception e) {
		myfile.open("Error_Generic.log", ios_base::app);
		myfile << "ERROR: Error parsing the PPLANs section" << endl;

		BOOST_FOREACH(ptree::value_type & v, pt)
		{
			myfile << v.first << v.second.data();
		}
		myfile.close();
		exit(0);
	}

	string plant;
	string MEPlant;
	locale loc;

	vector<string> plant_ME = split_string(part_revision, '_');
	if (plant_ME.size() == 2) {
		plant = plant_ME[0];
		//MEPlant = plant_ME[1];

		for (std::string::size_type i = 0; i < plant_ME[1].length(); ++i)
			MEPlant += std::toupper(plant_ME[1][i], loc);
	}
	string rev_l;
	string rev_n;
	parse_rev(wplan_rev, rev_n, rev_l);

	string dev_plan_rev = rev_l;

	//string plan_id = "PRC" + part_id + "_" + part_revision;
	string plan_name = part_id + "_" + part_revision;

	fileName = plan_name + "_WARNING_MESSAGES.log";

	// Automated Process Plan ID
	//cout << nLOC << "Plan ID: " << plan_id;
	cout << nLOC << "Plan Rev: " << dev_plan_rev << flush;

	wi_item myPrc;
	if (myPrc.getREV_t()) {
		//cout << nLOC << plan_id << "/" << dev_plan_rev << " already exists!";
		return;
	}

	wi_item meTarget(part_id, rev_l);
	if (!meTarget.getREV_t()) {
		myfile.open(fileName, ios_base::app);
		myfile << "ERROR: Part: " << part_id << "/" << rev_l << " does not exist or I cannot see it - exiting." << flush;
		myfile.close();
		exit(0);
	}
	vector<string> split_name;
	if (plan_creator != "") {
		split_name = split_string(plan_creator, ' ');
		if (split_name.size() == 2) {
			plan_creator = split_name[1] + ", " + split_name[0];
		}
	}

	//Owner not working properly...fix later - Dillon Scott
	//tag_t user_tag = get_persons_user_tags(plan_creator.c_str());
	//tag_t user1 = findUser("infodba");
	myPrc.instantiate("W4_ProcessGroup", plan_name, "My Description", true);
	myPrc.linkAssociate(meTarget);

	//prep directories
	prepDirs(myPrc.getPN(), myPrc.getREV());

	//process group attrs
	setProperty(myPrc.getPN_t(), "w4_PartNumber", part_id);
	setProperty(myPrc.getPN_t(), "object_desc", plan_desc);
	if (plant == "000") {
		//flag
		myfile.open(fileName, ios_base::app);
		myfile << "ERROR: Plant 000 is not a valid plant code...will need to fix post import." << endl;
		myfile.close();
	}
	setProperty(myPrc.getPN_t(), "w4_Plant", plant);
	setProperty(myPrc.getPN_t(), "w4_MEPlant", MEPlant);
	if (plan_prod_stat == "P") {
		//setReleaseStatus(myPrc.getPN_t(), "W4_RELEASED");
		setReleaseStatus(myPrc.getREV_t(), "W4_RELEASED");
	}
	else if (plan_prod_stat == "D") {
		//SetReleaseStatus(myPrc.getPN_t(), "");
	}
	else if (plan_prod_stat == "A") {
		//SetReleaseStatus(myPrc.getPN_t(), "");
	}
	//owning user
	/*
	if (plan_creator != "") {
		split_name = split_string(plan_creator, ' ');
		if (split_name.size() == 2) {
			plan_creator = split_name[1] + ", " + split_name[0];
		}
	}
	//Owner stuff not wortking properly. Fix later. -Dillon Scott
	//NOTE: if set here all object instantiated with the PP will
	//still have the old owner...
	
	user_tag = get_persons_user_tags(plan_creator.c_str());
	IFERR_REPORT(POM_set_owning_user(myPrc.getPN_t(), user_tag));
	IFERR_REPORT(POM_set_owning_user(myPrc.getREV_t(), user_tag));
	IFERR_REPORT(AOM_save_without_extensions(myPrc.getPN_t()));
	IFERR_REPORT(AOM_save_without_extensions(myPrc.getREV_t()));
	//mod user
	if (plan_rev_author != "") {
		split_name = split_string(plan_rev_author, ' ');
		if (split_name.size() == 2) {
			plan_rev_author = split_name[1] + ", " + split_name[0];
		}
	}
	user_tag = get_persons_user_tags(plan_rev_author.c_str());
	IFERR_REPORT(POM_set_owning_user(myPrc.getPN_t(), user_tag));
	IFERR_REPORT(POM_set_owning_user(myPrc.getREV_t(), user_tag));
	IFERR_REPORT(AOM_save_without_extensions(myPrc.getPN_t()));
	IFERR_REPORT(AOM_save_without_extensions(myPrc.getREV_t()));
	*/

	//modify_date_created(myPrc.getPN_t(), plan_create_date);
	//modify_date_created(myPrc.getREV_t(), plan_create_date);
	modify_date_released(myPrc.getPN_t(), plan_rev_date);
	modify_date_released(myPrc.getREV_t(), plan_rev_date);

	logical wplan_serial_num_req_bool = false;
	logical wplan_serial_num_trans_bool = false;
	if (wplan_serial_num_req == "YES") {
		wplan_serial_num_req_bool = true;
	}
	if (wplan_serial_num_trans == "YES") {
		wplan_serial_num_trans_bool = true;
	}
	verifyPartAttrs(meTarget.getPN_t(), wi_team, wplan_serial_num_req_bool, wplan_serial_num_trans_bool);

	//is this right???? - Dillon
	if (wplan_crit_sens == "YES") {
		setProperty(myPrc.getPN_t(), "w4_CriticalSensitive", "Critical");
		setProperty(myPrc.getREV_t(), "w4_CriticalSensitive", "Critical");
	}
	else {
		setProperty(myPrc.getPN_t(), "w4_CriticalSensitive", "Sensitive");
		setProperty(myPrc.getREV_t(), "w4_CriticalSensitive", "Sensitive");
	}


	//processGroupRevision attrs
	setProperty(myPrc.getREV_t(), "w4_EngPartNumber", part_id);
	setProperty(myPrc.getREV_t(), "object_desc", plan_desc);
	setProperty(myPrc.getREV_t(), "w4_Plant", plant);
	setProperty(myPrc.getREV_t(), "object_name", plan_name + "_" + wplan_rev);

	//setProperty(myPrc.getPN_t(), "w4_MEPlant", MEPlant);

	boostOperations(myPrc, pt);

	cout << nLOC << "Got Operations!";

	string new_path = path_g + "/" + myPrc.getPN() + ".json";

	if (std::rename(json_cimx_path.c_str(), new_path.c_str()) < 0) {
		std::cout << strerror(errno) << '\n';
	}

	/*
	string xml_path = "\\temp\\plans\\xml\\" + myPrc.getPN() + ".xml";
	string json_mpp_path = "\\temp\\plans\\xml\\" + myPrc.getPN() + ".json";

	write_plmxml_file(myPrc.getBVR_t(), xml_path);

	XMLParser xml;
	xml.InitIndex(xml_path);
	xml.toJSON(json_mpp_path);

	cout << nLOC << "Translating";
	string cmd = "\\temp\\plans\\tools\\xsl2json2.js \\temp\\plans\\xml\\" + myPrc.getPN() + " t7";
	system(cmd.c_str());
	*/
}


extern "C" int ITK_user_main(int argc, char* argv[])
{
	cout << "Starting Bom Load" << flush;

	for (int i = 0; i < argc; i++) {
		cout << nLOC << "Arg[" << i << "]: " << argv[i];
	}

	string target_id = argv[1];
	string logpath = "\\temp\\plans\\xml\\" + target_id + ".log";
	ofstream out(logpath.c_str());
	cout.rdbuf(out.rdbuf());

	cout << nLOC << "Starting..." << flush;

	int status = TCLogin();

	cout << nLOC << "Login Complete..." << flush;

	dispatch = false;


	int mode = 0;

	cout << nLOC << "Starting Main1..." << flush;

	switch (mode) {
	case 1:
		cout << T2U(NULLTAG);
		cout << nLOC << "Mode 1" << flush;
		//doTest1();
		break;

	case 2:
		cout << nLOC << "Mode 2";
		//doTest2();
		break;

	case 3:
		cout << nLOC << "Mode 3";
		//doTest3();
		break;

	case 4:
		cout << nLOC << "Mode 4";
		//doTest4();
		break;

	case 5:
		cout << nLOC << "Mode 5";
		//doTest5();
		break;

	default:
		cout << nLOC << "Mode Default" << flush;
		doPlan(argv[1]);

		/*
		wi_item step("STP00001569", "000");
		wi_dataset ds("test", "PDF", "PDF", "\\temp\\0125-00-0.pdf");

		ds.addFile("\\temp\\0125-00-0.pdf", NULL, "PDF_Reference");

		//step.linkWorkInstructions(ds);

		tag_t
			dataset = NULLTAG,
			imported_file = NULLTAG,
			* reference_objects = NULL;
		*/
		
		//wi_item item("119077", "E");
		

		//verifyPartAttrs(item.getPN_t(), "SHEET METAL CELL", "TRUE" , "TRUE");
		//setReleaseStatus(PP.getREV_t(), "W4_RELEASED");

		//setProperty(PP.getREV_t(), "object_name", "66993_800_PROD_3NC");
		//setProperty(PP.getPN_t(), "w4_Serialized", "TRUE");
		//setProperty(PP.getPN_t(), "w4_Serialized", "TRUE");

		/*
		setProperty(PP.getPN_t(), "w4_PartNumber", "111126");
		setProperty(PP.getPN_t(), "object_desc", "HP TURBINE ROTOR ASS'Y");
		setProperty(PP.getPN_t(), "w4_Plant", "800");
		setProperty(PP.getPN_t(), "w4_MEPlant", "PROD");
		setProperty(PP.getPN_t(), "w4_CriticalSensitive", "Critical");

		setProperty(PP.getREV_t(), "w4_EngPartNumber", "111126");
		setProperty(PP.getREV_t(), "object_desc", "HP TURBINE ROTOR ASS'Y");
		setProperty(PP.getREV_t(), "w4_Plant", "800");
		//setProperty(PP.getREV_t(), "w4_MEPlant", "PROD");
		setProperty(PP.getREV_t(), "w4_CriticalSensitive", "Critical");

		//oper
		wi_item oper("OP001278", "000");
		setTimeFormProps(oper.getREV_t(), "4", "4.4", "7.4", "4.23", "4.00", ".432");
		*/
	}

	cout << "\n\n Use GDE_ask_view_type to confirm that the bvrView type is acceptable";
	return status;
}



