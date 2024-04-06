#include <gtk/gtk.h>
#include <zmq.h>
#include "ck_types.h"
#include <thread>
#include <glib.h>

GtkStatusIcon *g_status_icon = nullptr;
void *g_socket_req = nullptr;
int call_server(int id)
{
     if (!g_socket_req)
     {
          fprintf(stderr, "socket_req is null!\n");
          return -1;
     }
     zmq_msg_t msg;
     zmq_msg_init_size(&msg, sizeof(CK_REQ));
     CK_REQ *data_req = (CK_REQ *)zmq_msg_data(&msg);
     data_req->magic = CK_MAGIC;
     data_req->id = id;
     int ret = zmq_sendmsg(g_socket_req, &msg, 0);
     if (ret < 0)
     {
          fprintf(stderr, "zmq_sendmsg failed!err=%s\n", zmq_strerror(zmq_errno()));
          zmq_msg_close(&msg);
          return -1;
     }
     zmq_msg_init(&msg);
     ret = zmq_recvmsg(g_socket_req, &msg, 0);
     if (ret < 0)
     {
          fprintf(stderr, "zmq_recvmsg failed!errno=%d\n", zmq_errno());
          zmq_msg_close(&msg);
          return -1;
     }
     CK_REP *data_rep = (CK_REP *)zmq_msg_data(&msg);
     if (data_rep->magic != CK_MAGIC || data_rep->id != id)
     {
          fprintf(stderr, "data rep invalid!magic=0x%lx id=%ld\n", data_rep->magic, data_rep->id);
          zmq_msg_close(&msg);
          return -1;
     }
     int val = data_rep->value;
     zmq_msg_close(&msg);

     // fprintf(stderr, "call server id(%d): ret(%d)!\n", id, val);
     return val;
}

static void set_tray_icon(CK_KEYBOARD_TYPE type)
{
     if (access("/usr/share/chrome_keyboard", R_OK) >= 0)
     {
          switch (type)
          {
          case CK_KEYBOARD_PC:
               gtk_status_icon_set_from_file(GTK_STATUS_ICON(g_status_icon), "/usr/share/chrome_keyboard/pc.png");
               break;
          case CK_KEYBOARD_CHROME:
               gtk_status_icon_set_from_file(GTK_STATUS_ICON(g_status_icon), "/usr/share/chrome_keyboard/chromebook.png");
               break;
          default:
               gtk_status_icon_set_from_file(GTK_STATUS_ICON(g_status_icon), "/usr/share/chrome_keyboard/error.png");
               break;
          }
     }
     else
     {
          switch (type)
          {
          case CK_KEYBOARD_PC:
               gtk_status_icon_set_from_file(GTK_STATUS_ICON(g_status_icon), "./res/pc.png");
               break;
          case CK_KEYBOARD_CHROME:
               gtk_status_icon_set_from_file(GTK_STATUS_ICON(g_status_icon), "./res/chromebook.png");
               break;
          default:
               gtk_status_icon_set_from_file(GTK_STATUS_ICON(g_status_icon), "./res/error.png");
               break;
          }
     }
}

static void on_tray_icon_activate(GtkWidget *widget, gpointer data)
{
     g_print("on_tray_icon_activate\n");

     CK_KEYBOARD_TYPE mode = (CK_KEYBOARD_TYPE)call_server(CK_FUNC_GET_KEYBOARD_TYPE);
     if (mode == CK_KEYBOARD_PC)
     {
          int ret = call_server(CK_FUNC_SWITCH_TO_CHROME_MODE);
          if (ret == 0)
          {
               return;
          }
     }
     else if (mode == CK_KEYBOARD_CHROME)
     {
          int ret = call_server(CK_FUNC_SWITCH_TO_PC_MODE);
          if (ret == 0)
          {
               return;
          }
     }
     set_tray_icon(CK_KEYBOARD_UNKOWN);
}

void on_mode_changed(GtkWidget *widget, CK_KEYBOARD_TYPE type)
{
     g_print("on_mode_changed %d\n", type);
     set_tray_icon(type);
}

gint g_signal_id_mode_chnaged = 0;

void _sub_proc()
{
     void *context = zmq_ctx_new();
     void *socket_sub = zmq_socket(context, ZMQ_SUB);
     int ret = zmq_connect(socket_sub, "tcp://localhost:" CK_ADDR_PUB);
     if (ret != 0)
     {
          fprintf(stderr, "start sub failed!\n");
          zmq_close(socket_sub);
          zmq_ctx_destroy(context);
          return;
     }
     zmq_setsockopt(socket_sub, ZMQ_SUBSCRIBE, "", 0);
     fprintf(stderr, "start sub succeed!port:%s\n", CK_ADDR_PUB);

     zmq_msg_t msg;
     zmq_msg_init(&msg);
     while (true)
     {
          int ret = zmq_recvmsg(socket_sub, &msg, 0);
          if (ret < 0)
          {
               break;
          }
          CK_REP *data_rep = (CK_REP *)zmq_msg_data(&msg);
          if (zmq_msg_size(&msg) != sizeof(CK_REP) || data_rep->magic != CK_MAGIC)
          {
               continue;
          }
          if (data_rep->id == CK_FUNC_GET_KEYBOARD_TYPE)
          {
               g_signal_emit_by_name(g_status_icon, "mode_changed", data_rep->value);
          }
     }
     zmq_msg_close(&msg);
     zmq_close(socket_sub);
     zmq_ctx_destroy(context);
}
void start_sub()
{
     std::thread([&]()
                 { while(1){
                    _sub_proc();
                    sleep(1);
               } })
         .detach();
}

void start_req(void *context)
{
     g_socket_req = zmq_socket(context, ZMQ_REQ);
     int val = 1;
     zmq_setsockopt(g_socket_req, ZMQ_REQ_CORRELATE, &val, sizeof(val));
     zmq_setsockopt(g_socket_req, ZMQ_REQ_RELAXED, &val, sizeof(val));
     int ret = zmq_connect(g_socket_req, "tcp://localhost:" CK_ADDR_REP);
     if (ret != 0)
     {
          fprintf(stderr, "req connect failed!err:%d,%s\n", zmq_errno(), zmq_strerror(zmq_errno()));
          zmq_close(g_socket_req);
          g_socket_req = nullptr;
          return;
     }
     fprintf(stderr, "start req succeed!port:%s ret=%d\n", CK_ADDR_REP, ret);

     ret = call_server(CK_FUNC_GET_KEYBOARD_TYPE);
     set_tray_icon((CK_KEYBOARD_TYPE)ret);
}

static void activate(GtkApplication *app, gpointer context)
{
     GtkWidget *window;

     window = gtk_application_window_new(app);
     gtk_window_set_title(GTK_WINDOW(window), "Window");
     gtk_window_set_default_size(GTK_WINDOW(window), 1, 1);
     gtk_container_set_border_width(GTK_CONTAINER(window), 0);
     g_status_icon = gtk_status_icon_new_from_file("./res/error.png");
     gtk_status_icon_set_visible(g_status_icon, true);
     g_signal_connect(G_OBJECT(g_status_icon), "activate", G_CALLBACK(on_tray_icon_activate), window);
     g_signal_connect(G_OBJECT(g_status_icon), "mode_changed", G_CALLBACK(on_mode_changed), nullptr);

     start_sub();
     start_req(context);
     gtk_widget_hide(window);
}

int main(int argc, char **argv)
{
     GtkApplication *app;
     int status;
     g_type_init();

     g_signal_id_mode_chnaged = g_signal_new("mode_changed",
                                             G_TYPE_OBJECT,
                                             G_SIGNAL_RUN_FIRST,
                                             0,
                                             NULL,
                                             NULL,
                                             g_cclosure_marshal_VOID__INT,
                                             G_TYPE_NONE,
                                             1,
                                             G_TYPE_INT);

     void *context = zmq_ctx_new();

     app = gtk_application_new("top.mydata.chrome_keyboard", G_APPLICATION_FLAGS_NONE);
     g_signal_connect(app, "activate", G_CALLBACK(activate), context);
     status = g_application_run(G_APPLICATION(app), argc, argv);
     g_object_unref(app);
     zmq_ctx_destroy(context);

     return status;
}