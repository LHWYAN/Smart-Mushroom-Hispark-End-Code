#include "common_def.h"
#include "soc_osal.h"
#include "app_init.h"
#include "stdio.h"
#include "mq2.h"

/**
 * @brief 烟雾传感器测试任务
 */
void *mq2_test_task(const char *arg)
{
    unused(arg);

    // 初始化 MQ-2 传感器 (ADC)
    if (mq2_init() != 0) {
        printf("MQ-2 INIT FAIL!\n");
        return NULL;
    }

    printf("MQ-2 TEST TASK START RUNNING...\n");

    while (1) {
        uint8_t smog = mq2_get_percentage();
        printf("Smog: %d %%\n", smog);
        
        // 超过一定浓度触发报警 (根据原版逻辑，值大于4即可触发报警)
        if (smog > 4) {
            printf("Smog Warn: Warn!\n");
        }
        
        osal_msleep(1000);
    }

    return NULL;
}

/**
 * @brief 任务入口函数
 */
static void mq2_task_entry(void)
{
    osal_task *task_handle = NULL;
    
    osal_kthread_lock();
    
    // 创建测试任务
    task_handle = osal_kthread_create((osal_kthread_handler)mq2_test_task,
                                      0,
                                      "Mq2TestTask",
                                      0x500);
    
    if (task_handle != NULL) {
        osal_kthread_set_priority(task_handle, 28);
        printf("MQ-2 test task create success!\n");
    } else {
        printf("MQ-2 test task create fail!\n");
    }
    
    osal_kthread_unlock();
}

/* 使用app_run宏注册应用程序入口 */
app_run(mq2_task_entry);
