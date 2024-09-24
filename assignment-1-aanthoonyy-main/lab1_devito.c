#include <stdlib.h>
#include <stdio.h>
#include <string.h>
// you can use additional headers as needed

typedef struct node
{
	char name[64]; // node's name string
	char type;
	struct node *child, *sibling, *parent;
} NODE;

NODE *root;
NODE *cwd;
char *cmd[] = {"mkdir", "rmdir", "ls", "cd", "pwd", "creat", "rm",
			   "reload", "save", "quit", 0}; // fill with list of commands
// other global variables
int test;

int initialize()
{
	root = (NODE *)malloc(sizeof(NODE));
	strcpy(root->name, "/");
	root->parent = root;
	root->sibling = 0;
	root->child = 0;
	root->type = 'D';
	cwd = root;
	printf("Filesystem initialized!\n");
}

void ls(NODE *node)
{
	if (node != NULL)
	{
		NODE *child = node->child;

		if (child == NULL)
		{
			// printf("cwd %s>\n", node->name);
			return;
		}

		// printf("cwd %s>\n", node->name);
		while (child != NULL)
		{
			printf("%c %s\n", child->type, child->name);
			child = child->sibling;
		}
	}
	else
	{
		printf("uh oh node is null something is wrong!!!!\n");
	}
}

void cd(char *dirName)
{

	NODE *pCur = cwd->child;
	if (dirName == NULL){
		cwd = root;
		return;
	}
	if (strcmp(dirName, "..") == 0)
	{
		// printf("going back\n");
		// printf("cwd> %s\n", dirName);
		if (cwd != NULL)
			cwd = cwd->parent;
		return;
	}

	if (pCur->type == 'F')
	{
		printf("can't CD a file\n");
		return;
	}
	while (pCur != NULL)
	{

		if (strcmp(pCur->name, dirName) == 0)
		{
			cwd = pCur;
			// printf("cwd> %s\n", dirName);
			
			return;
		}
		pCur = pCur->sibling;
	}
	printf("No such file or directory: %s\n", dirName);
}

void rmdir(char *dirName) {
    if (cwd == NULL) {
        printf("DIR %s does not exist!\n", dirName);
        return;
    }
	if (cwd->child == NULL){
        printf("DIR %s does not exist!\n", dirName);
        return;
    }
    NODE *pTemp = cwd->child, *pPrev = NULL;

    while (pTemp != NULL) {
        if (strcmp(pTemp->name, dirName) == 0) {
			if(pTemp->type == 'D'){
            if (pTemp->child != NULL) {
                printf("Cannot remove %s (not empty)!\n", dirName);
                return;
            } else {
                if (pPrev == NULL) {
                    cwd->child = pTemp->sibling;
                } 
				else {
                    pPrev->sibling = pTemp->sibling;
                }
                free(pTemp);
                return;
            }
			}
        }
        pPrev = pTemp;
        pTemp = pTemp->sibling;
    }
    printf("DIR %s does not exist!\n", dirName);
}

void pwd(NODE *node)
{
	NODE *pwdNode = node;
	if (node != NULL)
	{
		printf("%s", root->name);
		pwdNode = root->child;
		while (pwdNode != NULL && pwdNode != cwd->child)
		{
			printf("%s/", pwdNode->name);
			pwdNode = pwdNode->child;
		}
		printf("\n");
	}
}

void rm(char *fileName)
{
    NODE *pTemp = cwd->child, *pPrev = NULL;
    if (pTemp == NULL)
    {
        printf("FILE %s does not exist!\n", fileName);
        return;
    }

    while (pTemp != NULL)
    {
        if (strcmp(pTemp->name, fileName) == 0)
        {
            if (pTemp->type != 'F')
            {
                printf("Cannot remove %s (not a FILE)!\n", fileName);
                return;
            }
            if (pPrev == NULL)
            {
                cwd->child = pTemp->sibling;
            }
            else
            {
                pPrev->sibling = pTemp->sibling;
            }
            free(pTemp);
            return;
        }
        pPrev = pTemp;
        pTemp = pTemp->sibling;
    }

    printf("FILE %s does not exist!\n", fileName);
}

void save(NODE *node, FILE* fp)
{
	if (node == NULL)
	{
		printf("error: node is NULL\n");
		return;
	}

	if (fp == NULL)
	{
		return;
	}
	// printf("node name: %s, node parent: %s\n", node->name, node->parent->name);
	if (strcmp(node->name, "/") != 0)
		fprintf(fp, "%s,%s,%c\n", node->name, node->parent->name, node->type);

	NODE *child = node->child;
	while (child != NULL)
	{
		save(child, fp);
		child = child->sibling;
	}
	
}

NODE* searchTree(NODE *node, char *name) {
    if (node == NULL) {
        return NULL;
    }

    if (strcmp(node->name, name) == 0) {
        return node;
    }

    NODE *child = node->child;
    while (child != NULL) {
        NODE *found = searchTree(child, name);
        if (found != NULL) {
            return found;
        }
        child = child->sibling;
    }

    return NULL;
}

void reload(FILE* fp) {
    char buffer[256];
	char *dirName; 
	char *parentDirName;
	char *type;
	char *type2;
    NODE *parent;
	NODE *child;

    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        dirName = strtok(buffer, ",");
        parentDirName = strtok(NULL, ",");
        type = strtok(NULL, "\n");
		type2 = type;
        if (dirName != NULL && parentDirName != NULL) {
            if (strcmp(parentDirName, "/") == 0) {
                parent = root;
            } else {
                parent = searchTree(root, parentDirName);
                if (parent == NULL) {
                    continue;
                }
            }
            child = searchTree(root, dirName);
            if (child == NULL) { // make child then insert it
                child = (NODE *)malloc(sizeof(NODE));
                strcpy(child->name, dirName);
				child->type = type2[0];
				// strcpy(child->type, type2);
                child->child = NULL;
                child->sibling = NULL;
			if (parent->child == NULL) {
				parent->child = child;
			} 
			else {
				NODE* temp = parent->child;
					while (temp->sibling != NULL) {
						temp = temp->sibling;
						}
						temp->sibling = child;
					}
					child->parent = parent;
            }
        }
    }
}

void mkdir(char *namePart){
	NODE *dupe = searchTree(cwd->child, namePart);
    if (dupe != NULL) {
        printf("DIR %s already exists!\n", namePart);
        return;
    }
	NODE *newDir = (NODE *)malloc(sizeof(NODE));
	strcpy(newDir->name, namePart);
	newDir->type = 'D';
	newDir->parent = cwd;
	newDir->sibling = cwd->child;
	cwd->child = newDir;
	newDir->child = NULL;
}

void creat(char* namePart){
	NODE *dupe = searchTree(cwd->child, namePart);
    if (dupe != NULL) {
        printf("%s already exists!\n", namePart);
        return;
    }
	NODE *newFile = (NODE *)malloc(sizeof(NODE));
	strcpy(newFile->name, namePart);
	newFile->type = 'F';
	newFile->parent = cwd;
	newFile->sibling = cwd->child;
	cwd->child = newFile;
	newFile->child = NULL;
}

void quit(){

}
int main()
{
	initialize();
	// other initialization as needed
	char buffer[100];
	char *namePart = "";
	char *commandName;
	char *removeNull;
	int loop = 1;
	while (loop)
	{
		printf("$ ");
		fgets(buffer, sizeof(buffer), stdin);
		commandName = strtok(buffer, " ");
		namePart = strtok(NULL, " ");
		removeNull = strtok(namePart, "\n");
		removeNull = strtok(commandName, "\n");
		// printf("printing command: %s\n", commandName);
		// printf("printing name: %s\n", namePart);

		if (strcmp(commandName, cmd[0]) == 0)
		{
			mkdir(namePart);

		}
		else if (strcmp(commandName, cmd[1]) == 0)
		{
			rmdir(namePart);
		}
		else if (strcmp(commandName, cmd[2]) == 0)
		{
			ls(cwd);
		}
		else if (strcmp(commandName, cmd[3]) == 0)
		{
			cd(namePart);
		}
		else if (strcmp(buffer, cmd[4]) == 0)
		{
			pwd(cwd);
		}
		else if (strcmp(buffer, cmd[5]) == 0)
		{
			creat(namePart);
		}
		else if (strcmp(buffer, cmd[6]) == 0)
		{
			rm(namePart);
		}
		else if (strcmp(buffer, cmd[7]) == 0)
		{
			FILE *fp = fopen("fssim_devito.txt", "r");
			reload(fp);
			fclose(fp);
		}
		else if (strcmp(buffer, cmd[8]) == 0)
		{
			FILE *fp = fopen("fssim_devito.txt", "w+");
			save(root, fp);
			fclose(fp);
		}
		else if (strcmp(buffer, cmd[9]) == 0)
		{
			FILE *fp = fopen("fssim_devito.txt", "w+");
			save(root, fp);
			fclose(fp);
			loop = 0;
		}
		else
		{
			printf("Command not found!\n");
		}
	}
	return 0;
}
