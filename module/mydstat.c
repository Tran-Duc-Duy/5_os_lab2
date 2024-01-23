#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/debugfs.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/ktime.h>

#include <linux/sched.h>
#include <linux/sched/signal.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/netdevice.h> 
#include <net/net_namespace.h>
#define DEBUGFS_DIR_NAME "mydstat"
#define DEBUGFS_FILE_NAME "info"

#define INFO_BUFFER_SIZE 8192

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Tran Duc Duy");
MODULE_DESCRIPTION("Lab 2 - ITMO university");

static struct dentry *debugfs_dir;
static struct dentry *debugfs_file;

struct mydstat_info
{
// cpu *
  unsigned long usr;
  unsigned long sys;
  unsigned long idl;
  unsigned long wai;
  unsigned long stl;

//dsk
  unsigned long read;
  unsigned long writ;

//net
  unsigned long recv;
  unsigned long send;

//paging *
  unsigned long in;
  unsigned long out;

//sys *
  unsigned long intr;
  unsigned long csw;
};
struct DiskStats {
    int major;
    int minor;
    char device[32];
    unsigned long long reads_completed;
    unsigned long long reads_merged;
    unsigned long long sectors_read;
    unsigned long long read_time;
    unsigned long long writes_completed;
    unsigned long long writes_merged;
    unsigned long long sectors_written;
    unsigned long long write_time;
};

int set_running_processes(struct mydstat_info *);
int set_sleeping_processes(struct mydstat_info *);
unsigned long set_field_val(char *, char *);
int get_cpu_info(struct mydstat_info *);
int get_dsk_info(struct mydstat_info *);
int get_net_info(struct mydstat_info *);
int get_paging_info(struct mydstat_info *);
int get_system_info(struct mydstat_info *);

unsigned long get_field_val(char *path, char *field_name)
{
  struct file *file;
  loff_t pos = 0;
  int buffer_size = 16384;
  char *buf;
  ssize_t bytes_read;

  // opening file
  file = filp_open(path, O_RDONLY, 0);
  if (IS_ERR(file))
  {
    pr_err("error when opening proc/stat file\n");
    return -1;
  }

  // allocate memory for data
  buf = kmalloc(buffer_size, GFP_KERNEL);
  if (!buf)
  {
    pr_err("error allocation failed\n");
    return -1;
  }

  unsigned long value = -1;

  // reading file content
  memset(buf, 0, buffer_size);
  bytes_read = kernel_read(file, buf, buffer_size - 1, &pos);
  if (bytes_read < 0)
  {
    pr_err("error reading file\n");
    kfree(buf);
    filp_close(file, NULL);
    return -1;
  }

  // searching for fieldname
  char *line = buf;
  while (*line != '\0')
  {
    if (strncmp(line, field_name, strlen(field_name)) == 0)
    {
      sscanf(line + strlen(field_name), "%lu", &value);
      break;
    }
    line = strchr(line, '\n');
    if (line)
      line++;
    else
      break;
  }

  // free space
  kfree(buf);
  filp_close(file, NULL);
  return value;
}

//impl feature
int get_cpu_info(struct mydstat_info *mydstat)
{
  struct file *file;
  loff_t pos = 0;
  int buffer_size = 16384;
  char *buf;
  ssize_t bytes_read;

  // opening file
  file = filp_open("/proc/stat", O_RDONLY, 0);
  if (IS_ERR(file))
  {
    pr_err("error when opening proc/stat file\n");
    return -1;
  }

  // allocate memory for data
  buf = kmalloc(buffer_size, GFP_KERNEL);
  if (!buf)
  {
    pr_err("error allocation failed\n");
    return -1;
  }

  // reading file content
  memset(buf, 0, buffer_size);
  bytes_read = kernel_read(file, buf, buffer_size - 1, &pos);
  if (bytes_read < 0)
  {
    pr_err("error reading file\n");
    kfree(buf);
    filp_close(file, NULL);
    return -1;
  }

  long user, nice, system, idle, iowait, steal, guest;

  char *line = buf;
  sscanf(line, "cpu %ld %ld %ld %ld %ld %*ld %*ld %ld %ld %*ld", &user, &nice, &system, &idle, &iowait, &steal, &guest);

  long div = user + nice + system + idle + iowait + steal;
  long divo2 = div / 2;
  user = (user >= guest) ? user - guest : 0;

  mydstat->usr = (100*user + nice + divo2) / div;
  mydstat->sys = (100*system + divo2) / div;
  mydstat->idl = (100*idle + divo2) / div;
  mydstat->wai = (100*iowait + divo2) / div;
  mydstat->stl = (100*steal + divo2) / div;

  // free space
  kfree(buf);
  filp_close(file, NULL);
  return 1;
}

int get_dsk_info(struct mydstat_info *mydstat){
  struct file *file;
  loff_t pos = 0;
  int buffer_size = 16384;
  char *buf;
  ssize_t bytes_read;

  // opening file
  file = filp_open("/proc/diskstats", O_RDONLY, 0);
  if (IS_ERR(file))
  {
    pr_err("error when opening proc/diskstats file\n");
    return -1;
  }

  // allocate memory for data
  buf = kmalloc(buffer_size, GFP_KERNEL);
  if (!buf)
  {
    pr_err("error allocation failed\n");
    return -1;
  }

  // reading file content
  memset(buf, 0, buffer_size);
  bytes_read = kernel_read(file, buf, buffer_size - 1, &pos);
  if (bytes_read < 0)
  {
    pr_err("error reading file\n");
    kfree(buf);
    filp_close(file, NULL);
    return -1;
  }
  // todo 
  int num_devices = 0;
    char* line = buf;
    while (*line) {
        struct DiskStats disk;
        if (sscanf(line, "%d %d %31s %llu %llu %llu %llu %llu %llu %llu %llu",
            &disk.major, &disk.minor, disk.device,
            &disk.reads_completed, &disk.reads_merged, &disk.sectors_read, &disk.read_time,
            &disk.writes_completed, &disk.writes_merged, &disk.sectors_written, &disk.write_time) == 11) {
            // In thông tin đọc và ghi cho thiết bị
            mydstat->read =disk.reads_completed;
            mydstat->writ =disk.writes_completed;

            num_devices++;
        }

        line = strchr(line, '\n');
        if (!line)
            break;
        line++;
    }

  // free space
  kfree(buf);
  filp_close(file, NULL);
  return num_devices;
}


int get_net_info(struct mydstat_info *mydstat) {
    struct file *file;
    loff_t pos = 0;
    int buffer_size = 16384;
    char *buf;
    ssize_t bytes_read;

    // Open the /proc/net/dev file
    file = filp_open("/proc/net/dev", O_RDONLY, 0);
    if (IS_ERR(file)) {
        pr_err("Failed to open /proc/net/dev\n");
        return -1;
    }

    // Allocate memory for data
    buf = kmalloc(buffer_size, GFP_KERNEL);
    if (!buf) {
        pr_err("Memory allocation failed\n");
        return -1;
    }

    // Read file content
    memset(buf, 0, buffer_size);
    bytes_read = kernel_read(file, buf, buffer_size - 1, &pos);
    if (bytes_read < 0) {
        pr_err("Error reading file\n");
        kfree(buf);
        filp_close(file, NULL);
        return -1;
    }

    // Parse network statistics
    char *line = buf;
    while (*line) {
        if (strchr(line, ':')) {
            char devname[IFNAMSIZ];
            char recv_bytes_str[20];  // Assuming the max length of recv_bytes is 20
            char send_bytes_str[20];  // Assuming the max length of send_bytes is 20
            unsigned long long recv_bytes, send_bytes;

            // Extract the device name and statistics
            if (sscanf(line, "%[^:]:%[^ ] %*u %*u %*u %*u %*u %*u %*u %[^ ]", devname, recv_bytes_str, send_bytes_str) == 3) {
                // Check if the device name is not "lo" (loopback)
                if (strcmp(devname, "lo") != 0) {
                    // Remove ':' from recv_bytes_str
                    char *colon = strchr(recv_bytes_str, ':');
                    if (colon) {
                        strncpy(recv_bytes_str, colon + 1, sizeof(recv_bytes_str));
                    }

                    // Convert the string representation of bytes to unsigned long long
                    kstrtoull(recv_bytes_str, 10, &recv_bytes);
                    kstrtoull(send_bytes_str, 10, &send_bytes);

                    // Assign the network statistics to the mydstat_info structure
                    mydstat->recv = recv_bytes;
                    mydstat->send = send_bytes;

                    break;
                }
            }
        }

        line = strchr(line, '\n');
        if (!line)
            break;
        line++;
    }

    // Free memory
    kfree(buf);
    filp_close(file, NULL);

    return 0;
}
int get_paging_info(struct mydstat_info *mydstat)
{
  s64 uptime;
  uptime = ktime_divns(ktime_get_coarse_boottime(), NSEC_PER_SEC);

  mydstat->in = get_field_val("/proc/vmstat", "pgpgin") / uptime / 8;
  mydstat->out = get_field_val("/proc/vmstat", "pgpgout") / uptime / 8;
  return 1;
}

int get_system_info(struct mydstat_info *mydstat)
{
  s64 uptime;
  uptime = ktime_divns(ktime_get_coarse_boottime(), NSEC_PER_SEC);

  mydstat->intr = get_field_val("/proc/stat", "intr") / uptime / 8;
  mydstat->csw = get_field_val("/proc/stat", "ctxt") / uptime / 8;

  return 1;
}


ssize_t mydstat_read(struct file *file, char __user *user_buf, size_t count, loff_t *ppos)
{
  char buf[INFO_BUFFER_SIZE];
  int total_len = 0;

  struct mydstat_info info;
  get_cpu_info(&info);
  get_dsk_info(&info);
  get_net_info(&info);
  get_paging_info(&info);
  get_system_info(&info);

  total_len += snprintf(buf, INFO_BUFFER_SIZE - total_len,
                        "usr: %lu\n"
                        "sys: %lu\n"
                        "idl: %lu\n"
                        "wai: %lu\n"
                        "stl: %lu\n"
                        "------\n"
                        "read: %lu\n"
                        "writ: %lu\n"
                        "------\n"
                        "recv: %lu\n"
                        "send: %lu\n"
                        "------\n"
                        "in: %lu\n"
                        "out: %lu\n"
                        "------\n"
                        "intr: %lu\n"
                        "csw: %lu\n"
                        "------\n",
                        info.usr,
                        info.sys,
                        info.idl,
                        info.wai,
                        info.stl,
                        info.read,
                        info.writ,
                        info.recv,
                        info.send,
                        info.in,
                        info.out,
                        info.intr,
                        info.csw
                        );

  return simple_read_from_buffer(user_buf, count, ppos, buf, total_len);
}

static int mydstat_open(struct inode *inode, struct file *file)
{
  return 0;
}

static int mydstat_release(struct inode *inode, struct file *file)
{
  return 0;
}

static const struct file_operations mydstat_fops = {
    .read = mydstat_read,
    .open = mydstat_open,
    .release = mydstat_release,
};

static int __init mydstat_init(void)
{
  debugfs_dir = debugfs_create_dir(DEBUGFS_DIR_NAME, NULL);
  if (!debugfs_dir)
  {
    pr_err("failed to create debugfs directory\n");
    return -ENOMEM;
  }

  debugfs_file = debugfs_create_file(DEBUGFS_FILE_NAME, 0644, debugfs_dir, NULL, &mydstat_fops);
  if (!debugfs_file)
  {
    pr_err("failed to create debugfs file\n");
    debugfs_remove(debugfs_dir);
    return -ENOMEM;
  }

  pr_info("mydstat module loaded\n");
  return 0;
}

static void __exit mydstat_cleanup(void)
{
  debugfs_remove_recursive(debugfs_dir);
  pr_info("mydstat module unloaded\n");
}

module_init(mydstat_init);
module_exit(mydstat_cleanup);
