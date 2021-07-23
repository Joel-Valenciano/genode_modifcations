#include <base/heap.h>
#include <base/component.h>

#include <lwip/tcp.h>
#include <lwip/genode_init.h>

#include <timer_session/connection.h>
	
uint16_t local_port = 40000;
uint16_t remote_port = 40000;

err_t start(void* arg, tcp_pcb* pcb, err_t err) {
	Genode::log("Connected to Something?");
	tcp_write(pcb, "awefwe", 7, 0);
	Genode::log(arg, (uint32_t) err);
	return ERR_OK;
}

err_t recv(void* arg, tcp_pcb* pcb, pbuf* p, err_t err) {
	Genode::log("Received packet.", arg, pcb, p, (int) err);
	return err;
}

void Component::construct(Genode::Env& env) {
	ip_addr_t ip;
	tcp_pcb* pcb;
	err_t err;

	Genode::Heap heap(env.ram(), env.rm());
	Timer::Connection ip_timer(env, env.ep());
	Lwip::genode_init(heap, ip_timer);
	
	IP_ADDR4(&ip, 8, 8, 8, 8);

	Genode::log(100);
	//Genode::log(pcb, env.pd().avail_ram());

	Timer::Connection sleep_timer(env, env.ep());
	while(1) {
		pcb = tcp_new();
		err = tcp_bind(pcb, IP4_ADDR_ANY, local_port);
		Genode::log((int) err);
		
		err = tcp_connect(pcb, &ip, remote_port, start);
		tcp_recv(pcb, recv);
		if(err != ERR_OK) {
			Genode::log("Retrying connection. ", (int) err);
			sleep_timer.msleep(1000);
			tcp_close(pcb);
		} else {
			break;
		}
	}
}

