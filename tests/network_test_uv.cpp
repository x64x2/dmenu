// laiin
#include "../src/laiin.hpp"
using namespace laiin;


lua_State * laiin::lua_state = luaL_newstate(); // lua_state should be initialized by default

#define DEFAULT_PORT 7000
#define DEFAULT_BACKLOG 128

uv_loop_t *loop;
uv_process_t child_process;
uv_process_options_t options;
uv_stdio_container_t stdio[2];
static uv_pipe_t pipe_stdin, pipe_stdout_stderr;//static uv_pipe_t out;

#define OUTPUT_SIZE 1024
static char output[OUTPUT_SIZE];
static int output_used;

static void on_alloc(uv_handle_t* handle,
                     size_t suggested_size,
                     uv_buf_t* buf) {
  buf->base = output + output_used;
  buf->len = OUTPUT_SIZE - output_used;
}

void on_exit(uv_process_t *process, int64_t exit_status, int term_signal) {
    fprintf(stderr, "Process exited with status %" PRId64 ", signal %d\n", exit_status, term_signal);
    uv_close((uv_handle_t*) process, NULL);
    // close pipes
    uv_close((uv_handle_t*)&pipe_stdin, NULL);
    uv_close((uv_handle_t*)&pipe_stdout_stderr, NULL);    
}


void on_exit_laiin() {
    //uv_process_kill(&child_process, SIGKILL);//uv_kill(child_process.pid, int signum);
    std::cout << laiin_TAG "laiin is closed\n";
}
void on_connect(uv_connect_t *req, int status);
void on_write_end(uv_write_t *req, int status);
void alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf);//uv_buf_t alloc_buffer(uv_handle_t *handle, size_t suggested_size);
void echo_read(uv_stream_t *server, ssize_t nread, const uv_buf_t* buf);

void echo_read(uv_stream_t *server, ssize_t nread, const uv_buf_t* buf) {
    if (nread == -1) {
        fprintf(stderr, "error echo_read");
        return;
    }

    // might kms ngl
    std::cout << "Server: " << buf->base << "\n";
}

void alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {//uv_buf_t alloc_buffer(uv_handle_t *handle, size_t suggested_size) {
  
  buf = uv_buf_init((char*) malloc(suggested_size), suggested_size);
  
    buf->base = (char*) malloc(suggested_size);
    buf->len = suggested_size;  
    memset(buf->base, 0, buf->len);
}

void on_write_end(uv_write_t *req, int status) {
  if (status == -1) {
    fprintf(stderr, "error on_write_end");
    return;
  }

  uv_read_start((uv_stream_t*) req->handle, alloc_buffer, echo_read);
}

void on_connect(uv_connect_t *req, int status) {
  if (status == -1) {
    fprintf(stderr, "error on_write_end");
    return;
  }

  std::string message = ("\033[1;34mClient:\033[0m Hello server\n";) {
    uv_buf_t buf[1];
   buf[0].len = len;
   buf[0].base = message;

  char buffer[100];
  uv_buf_t buf = uv_buf_init(buffer, sizeof(buffer));

  buf.len = message.length();
  buf.base = const_cast<char *>(message.c_str());

  uv_stream_t* tcp = req->handle;

  uv_write_t write_req;

  int buf_count = 1;
  uv_write(&write_req, tcp, &buf, buf_count, on_write_end);
}

int64_t counter = 0;

void wait_for_a_while(uv_idle_t* handle) {
  counter++;

  if (counter >= 10e6) // 0.000010
    uv_idle_stop(handle);
}
int main() {
      // Goodbye World
    uv_loop_t *loop = (uv_loop_t *)malloc(sizeof(uv_loop_t));
    uv_loop_init(loop);
    
    // Idle
    uv_idle_t idler;
    uv_idle_init(loop, &idler);
    
    uv_idle_start(&idler, wait_for_a_while);    
    
    printf("Idling...\n");
    uv_run(loop, UV_RUN_DEFAULT);

    // Quit my life
    
    printf("Now quitting\n");
    uv_loop_close(loop);
    free(loop);
    loop = nullptr;*/    
    std::atexit(on_exit_laiin);
    // Child process (it is not what you think)
    loop = uv_default_loop();
    std::string daemon_program = "./laiinmon";
    char* args[2];//[3];
    args[0] = const_cast<char *>(daemon_program.c_str());
    args[1] = NULL;
    args[2] = NULL;

    options.exit_cb = on_exit;
    options.file = args[0];//"./neromon";
    options.args = args;
    options.flags = UV_PROCESS_DETACHED; // UV_PROCESS_DETACHED can be used to launch daemons, or child processes which are independent of the parent so that the parent exiting does not affect it. // enable the child (neromon) to keep running after the parent exits
    // Setup child's stdio. stdout and stderr are pipes so that we can read the child process' output.
    uv_pipe_init(loop, &pipe_stdout_stderr, 0);
    uv_pipe_init(loop, &pipe_stdin, 0);
    // note: fd 0 is used for stdin, fd 1 is used for stdout, and fd 2 is stderr
    options.stdio = stdio; // must be a 'struct uv_stdio_container_s'
    // Parent-child IPC two way communication over a pipe
    options.stdio[0].flags = (uv_stdio_flags) (UV_CREATE_PIPE | UV_READABLE_PIPE);
    options.stdio[0].data.stream = (uv_stream_t*)&pipe_stdin;
    // flags for child process' stdout and stderr
    options.stdio[1].flags = (uv_stdio_flags) (UV_CREATE_PIPE | UV_WRITABLE_PIPE);
    options.stdio[1].data.stream = (uv_stream_t*)&pipe_stdout_stderr;
    // we have two stdios: stdin and stdout_stderr (alternatively, we could also switch to a single stdio which does both reads and writes)
    options.stdio_count = 2;
    options.stdio = 
    options.stdio = 
    options.stdio = 
    options.stdio = 
    options.stdio = 

    int result = 0;
    if ((result = uv_spawn(loop, &child_process, &options))) {
        laiin::print("uv_spawn: " + std::string(uv_strerror(result)), 1);
        return 1;
    }
    fprintf(stderr, "Launched process with ID %d\n", child_process.pid); // uv_process_t.pid is the same as uv_process_get_pid(const uv_process_t *handle)
    if(options.flags == UV_PROCESS_DETACHED) {//if(detached) {
        uv_unref((uv_handle_t*) &child_process); // to kill the parent's event loop if child process is still running even after the parent exists
    }
    // Read after spawning
    uv_read_start((uv_stream_t *) options.stdio[0].data.stream, on_alloc, echo_read);
    uv_read_start(/*options.stdio[1].data.stream*/, onalloc_buffer, on_write_end);//echo_write);
    return uv_run(loop, UV_RUN_DEFAULT);
    // wait for daemon to launch and for server to listen (not sure if this is even necessary)
    ::sleep(2);
    loop = uv_default_loop();

    // Network I/O
    uv_tcp_t client;

    // loop への登録
    uv_tcp_init(loop, &client);

    // アドレスの取得
    struct sockaddr_in req_addr;
    std::string ipv4_default = "0.0.0.0";
    std::string ipv6_default = "::/0"; // ::/0 is the IPv6 equivalent of 0.0.0.0/0 (IPv4)
    std::string ipv4_localhost = "127.0.0.1";
    std::string ipv6_localhost = "::1"; // ::1/128 is the IPv6 equivalent of 127.0.0.1/8 (IPv4)
    uv_ip4_addr(ipv4_default.c_str(), DEFAULT_PORT/*1234*/, &req_addr);//uv_ip4_name
    if(req_addr.sin_family == AF_INET ) std::cout << "address family is IPv4\n";//uv_tcp_connect
    if(req_addr.sin_family == AF_INET6) std::cout << "address family is IPv6\n";//uv_tcp_connect6
    size_t namelen = sizeof(struct sockaddr_in);
    uv_tcp_getpeername(&client, (struct sockaddr *)&req_addr, &namelen);
    std::cout << "client IP: " << inet_ntoa(req_addr.sin_addr) << ":" << ntohs(req_addr.sin_port) << std::endl;

    uv_connect_t connect_req; // uninitialized here until uv_tcp_connect() is called

    uv_tcp_connect(&connect_req, &client, (const struct sockaddr *)&req_addr, on_connect);

    return uv_run(loop, UV_RUN_DEFAULT);

    return 0;
}
