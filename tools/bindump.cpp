/*
 * bindump v0.1 - simple program to iterate over binder debug data
 *                and display who uses what
 *
 *   From: "Android Internals - A Confectioner's Cookbook"
 * 
 *         http://www.newandroidbook.com/
 *
 *         By Jonathan Levin, 08/2014
 *
 *  Loosely based on "service" from AOSP. Yours to use and abuse
 * 
 * Not production worthy code. Quick and dirty, to illustrate a point. C'est tout.
 *
 */

#include <binder/Parcel.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>
#include <binder/TextOutput.h>
#include <fcntl.h>
#include <dirent.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>

using namespace android;

#define DETAILS_MASK_OWNER	0x1
#define DETAILS_MASK_USERS	0x2
void writeString16(Parcel& parcel, const char* string)
{
    if (string != NULL)
    {
        parcel.writeString16(String16(string));
    }
    else
    {
        parcel.writeInt32(-1);
    }
}

// get the name of the generic interface we hold a reference to
static String16 get_interface_name(sp<IBinder> service)
{
    if (service != NULL) {
        Parcel data, reply;
        status_t err = service->transact(IBinder::INTERFACE_TRANSACTION, data, &reply);
        if (err == NO_ERROR) {
            return reply.readString16();
        }
    }
    return String16();
}

static String8 good_old_string(const String16& src)
{
    String8 name8;
    char ch8[2];
    ch8[1] = 0;
    for (unsigned j = 0; j < src.size(); j++) {
        char16_t ch = src[j];
        if (ch < 128) ch8[0] = (char)ch;
        name8.append(ch8);
    }
    return name8;
}

int g_verbose = 0;

sp<IServiceManager> sm ;

int getNodeForService(const char *ServiceName)
{

    sp<IBinder> service = sm->checkService(String16(ServiceName));
    char binderDebugData[1024];
    int rc = 0 ;

    if (service != NULL)
		   {
			// We have the service - sleep here for a sec
                        String16 ifName = get_interface_name(service);
			aout <<ifName ;
			char name[200];
			sprintf (name,"/sys/kernel/debug/binder/proc/%d", getpid());
			int fd = open (name, O_RDONLY);
			if (fd < 0 ) { 
				perror ("Can't open Binder debug data\n");
				exit(1);
				     }
			rc = read (fd, binderDebugData, 1024);
			

			if (rc > 0)
			{
				// look for node
				if (g_verbose)
				fprintf (stderr,"%s\n", binderDebugData);
				
				// First node entry is the context mgr
				char *node = strstr (binderDebugData, "node");
				if (!node)
				{
				fprintf (stderr, "Didn't find any node refs in debug data. This is odd. Maybe format changed!\n");
				exit(3);
				}
				// Otherwise, we need the *next* node ref
				node = strstr (node + 1, "node");
				if (!node)
				{
				fprintf (stderr, "Didn't find second node refs in debug data. This is odd. Maybe format changed!\n");
				}
				int nodeNum= 0;
				rc = sscanf (node + 4, "%d", &nodeNum);
				if (rc != 1)
				{
				fprintf(stderr,"Can't figure out the node ref\n");
				
				exit(6);
				}

				printf ("Service: %s node ref: %d\n", ServiceName, nodeNum);
				return (nodeNum);
			}

		   }
     else printf ("Service '%s' not found - use service list to get list of available services\n", ServiceName);


}

char *getProcName (char *pid)
{
	// Attempt to open /proc/pid/cmdline
	static char pathName[1024];
	sprintf (pathName, "/proc/%s/cmdline", pid);

	int fd  = open (pathName, O_RDONLY);
	if (fd < 0 ) return ((char *) "??");
	
	read (fd, pathName, 1024);
	close(fd);
	// cmdline is argv[] and null terminated. that's a bonus.
	return (pathName);

}

void findUsersOfNode (int nodeNum, int DetailsMask)
{
	// Iterate over all nodes in /sys/kernel/debug/binder
	// and find users of this node

       DIR *dirp = opendir("/sys/kernel/debug/binder/proc");
       struct dirent *dirent;

       char nodeInFile[80];
       sprintf (nodeInFile, "node %d", nodeNum);

       while (dirent = readdir(dirp))
	{
	    char path[10240];
	    memset(path,'\0',10240);
	    sprintf(path,"/sys/kernel/debug/binder/proc/%s",
		          dirent->d_name);

	    int fd = open (path, O_RDONLY);
	    if (fd < 0) { /* this could happen if a process terminates  */
			  /* by the time we get to open it. So no biggy */
			  continue;
			}
	     
	    int rc = read (fd, path, 10240); // reuse path, what the heck :-) 

	    if (rc < 0) { /* again, this could be normal.. */
			  continue;
 			}

            // For users, we look for a line of the form
            //  ref 4: desc 0 node %d s 1 w 1 d  ...
            // For owners, we look for a line of the form
            // node %d: u002d70b0 c002d7704 ... blah blah... proc pid1 pid2 ...

	    char *p = path;

	    while (p = strstr (p, nodeInFile)) {
	
		switch (p[strlen(nodeInFile)])
		{
		    case ' ': // User
			if (atoi(dirent->d_name) != getpid())
			{
			if ( (DetailsMask & DETAILS_MASK_USERS)) printf ("User:  PID %5s\t%s\n", dirent->d_name,getProcName(dirent->d_name));
			}
			
			break;

		    case ':': // Owner
			if ( (DetailsMask & DETAILS_MASK_OWNER))
			printf ("Owner: PID %5s\t%s\n", dirent->d_name, getProcName(dirent->d_name));
			break;
		    default:  // False Positive (part of longer num, probably)
			;;
		} // end switch
		p++;
		
	    } // end while

	    
	    close (fd);
		
	};


	
} 

#define MAX_BINDER_DATA_SIZE	(1024 *1024)
void enumerateNodes (int pid)
{

	char name[80];
	char buf[MAX_BINDER_DATA_SIZE];
	// Do we even have a PID?
	sprintf (name, "/proc/%d", pid);
	if (access (name, X_OK) != 0)
		{
			fprintf (stderr, "PID %d - no such process or thread\n",
			pid);
			return;
		}
	sprintf (name,"/sys/kernel/debug/binder/proc/%d", pid);
	int fd = open (name, O_RDONLY);
	if (fd < 0) { 
		sprintf (name, "%d", pid);
		fprintf (stderr, "PID %d (%s) is not using Binder\n", pid,getProcName(name));
		 return;

		}
	int rc = read (fd, buf, MAX_BINDER_DATA_SIZE);
	
	if (rc < 0) {
		fprintf (stderr, "Unable to read Binder debug data for pid %d\n",	pid);

				 return; }
	// Otherwise we have the data...
	
 	return ;
	char *firstRef = strstr(buf, "ref ");
	char *ref = firstRef;
	char *eol = NULL;
	while (ref)
	{
		eol = strchr (ref, '\n');
		if (!eol) { fprintf (stderr,"Binder data for PID %d malformed!\n", pid);
			     return;}
		
		*eol = '\0';
	//	printf ("Got Ref %s\n", ref);

		ref  = strstr (eol + 1, "ref ");
	}


} // enumerateNodes


int main(int argc, char* const argv[])
{
    sm = defaultServiceManager();
    int detailsMask = 0xFF;

    if (argc < 2)
	{
		fprintf (stderr, "Usage: %s [owner|users] _servicename_\n", argv[0]);
		fprintf (stderr, "Can also use \"all\" for all services\n");
		exit(11);
	}


    if (argc == 3)
	{
		if (strcmp(argv[1], "owner") == 0) detailsMask = DETAILS_MASK_OWNER;
		if (strcmp(argv[1], "users") == 0) detailsMask = DETAILS_MASK_USERS;
		
		   

	}

    if (strcmp(argv[argc-1], "all") == 0)
	{
		int s = 0;
	 	// First enumerate services, then for each
     		Vector<String16> services = sm->listServices();
		for (s = 0; 
		     s < services.size(); 
		     s++)
			{
			    int nodeNum = getNodeForService (String8(services[s]));
			    findUsersOfNode(nodeNum,detailsMask);

			}
		return (0);

	}
  
    // Is it a PID?
    int pid = atoi(argv[argc - 1 ]);
    if (pid) /* != 0 */
	{
		enumerateNodes(pid);
		return (0);

	};

    // Otherwise, it's a service name..
    int nodeNum = getNodeForService (argv[argc-1]);
    findUsersOfNode(nodeNum,detailsMask);
    return (0);

}
