#include <base/log.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <libc/component.h>
//#include <libc/args.h>
#include <log_session/connection.h>

int main(int argc, char** argv) {
	Genode::log("0awefew;");

	struct sockaddr_in local_saddr;
	struct sockaddr_in saddr;
	int err = 0;
	int sockfd = 0;

	Genode::log(argc, argv);
	Genode::log("waojawoefijwoefjwoiefjweiojewiofjeio;");
	
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	printf("%d\n", sockfd);

	local_saddr.sin_family = AF_INET;
	local_saddr.sin_port = htons(41000);
	local_saddr.sin_addr.s_addr = INADDR_ANY;

	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(80);
	saddr.sin_addr.s_addr = inet_addr("8.8.8.8");

	printf("%d, %d\n", local_saddr.sin_port, saddr.sin_port);

	err = bind(sockfd, (struct sockaddr*) &local_saddr, sizeof(struct sockaddr_in));
	printf("%d\n", err);
	bool connected = false;
	while(!connected) {
		err = connect(sockfd, (struct sockaddr*) &saddr, sizeof(struct sockaddr_in));
		printf("%d\n", err);
		if(err == 0) {
			break;
		}
		usleep(500 * 1000);
	}

	err = write(sockfd, "GET /\n\n", 8);
	printf("%d\n", err);
	return 0;
}

//int run(Genode::Env& env, int argc, char** argv);

//int run(Genode::Env&);

//void Libc::Component::construct(Libc::Env& env) {
//	Genode::log(100);
//	Genode::log(env.pd().cap_quota());
	//int argc = 0;
	//char** argv = nullptr;
	//char** envp = nullptr;

	//populate_args_and_env(env, argc, argv, envp);
	//exit(run(env, argc, argv));
//	run(env);
//}

//int run(Genode::Env& env) {
int run(Genode::Env& env, int argc, char** argv) {
	Libc::with_libc([&] {
		Genode::log(argc, argv);
//		int* a = 0;
//		*a = 100;
//		Genode::Log_connection log(env);
//		log.write("af23f2o3oijfe23ofij32o3joi");
		Genode::log(env.pd().cap_quota());
		struct sockaddr_in local_saddr;
		struct sockaddr_in saddr;
		int err = 0;
		int sockfd = 0;

	//Genode::log(sockfd);
	//FILE* log = fopen("/dev/log", "w");
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	//fprintf(log, "%d\n", sockfd);
		printf("%d\n", sockfd);

		local_saddr.sin_family = AF_INET;
		local_saddr.sin_port = htons(41000);
		local_saddr.sin_addr.s_addr = INADDR_ANY;

		saddr.sin_family = AF_INET;
		saddr.sin_port = htons(80);
		saddr.sin_addr.s_addr = inet_addr("8.8.8.8");

		printf("%d, %d\n", local_saddr.sin_port, saddr.sin_port);

		err = bind(sockfd, (struct sockaddr*) &local_saddr, sizeof(struct sockaddr_in));
		Genode::log(err);
		printf("%d\n", err);
		err = connect(sockfd, (struct sockaddr*) &saddr, sizeof(struct sockaddr_in));
		Genode::log(err);
		printf("%d\n", err);

		err = write(sockfd, "GET /\n\n", 8);
	//Genode::log(err);
		printf("%d\n", err);
	});
	
	return 0;
}
