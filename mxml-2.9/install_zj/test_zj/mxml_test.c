#include "mxml.h"


void creat_mxml()
{
    FILE *fp;
    
    
    mxml_node_t *xml;
    mxml_node_t *data;
    mxml_node_t *node;
    mxml_node_t *group;
    
    xml = mxmlNewXML("1.0");
    data = mxmlNewElement(xml, "data");
	mxmlNewText(data, 0, "val1");
    //node = mxmlNewElement(data, "node");    
    //mxmlNewText(node, 0, "val1");
    


    fp = fopen("test.xml", "w+");
    mxmlSaveFile(xml, fp, MXML_NO_CALLBACK);

    fclose(fp);
    mxmlDelete(xml);
}

int find_mxml()
{
    FILE *fp;
    mxml_node_t *tree,*node;

    fp = fopen("test.xml", "r");
    tree = mxmlLoadFile(NULL, fp, MXML_TEXT_CALLBACK);
    fclose(fp);

    mxml_node_t *id,*password;

	/* MXML_DESCEND_FIRST 只遍历子节点，且找到后不再遍历 */
	node = mxmlFindElement(tree, tree, "data",NULL, NULL, MXML_DESCEND_FIRST);
    printf(" attr:%s \n",mxmlElementGetAttr(node,"attr"));


	node = mxmlFindElement(node, node, "node1",NULL, NULL, MXML_DESCEND_FIRST);

    printf(" attr:%s \n",mxmlElementGetAttr(node,"attr"));

	

	node = mxmlFindElement(node, node, "node2",NULL, NULL, MXML_DESCEND);

    printf(" attr:%s \n",mxmlElementGetAttr(node,"attr"));






    mxmlDelete(tree);

    return 0 ;
}


int main(int argc,char *argv[])
{   
    int ret;
    int     i_opt;
    
	creat_mxml();
#if 0    
    while ((i_opt = getopt(argc, argv, "h")) != -1) {
        switch (i_opt) {
            case 'h':
                test_usage();
                break;

            default:
                test_usage();
        }
    }
#endif	
}
