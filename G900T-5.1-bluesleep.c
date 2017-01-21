words
---
diff --git a/drivers/bluetooth/bluesleep_fpga.c b/drivers/bluetooth/bluesleep_fpga.c
index 77f2de98edf..d52d334a7ed 100644
--- a/drivers/bluetooth/bluesleep_fpga.c
+++ b/drivers/bluetooth/bluesleep_fpga.c
@@ -76,10 +76,12 @@
 #define POLARITY_LOW 0
 #define POLARITY_HIGH 1

-/* enable/disable wake-on-bluetooth */
-#define BT_ENABLE_IRQ_WAKE 1
-
-#define BT_BLUEDROID_SUPPORT 1
+enum msm_hs_clk_states_e {
+	MSM_HS_CLK_PORT_OFF,     /* port not in use */
+	MSM_HS_CLK_OFF,          /* clock disabled */
+	MSM_HS_CLK_REQUEST_OFF,  /* disable after TX and RX flushed */
+	MSM_HS_CLK_ON,           /* clock enabled */
+};

 struct bluesleep_info {
 	unsigned host_wake;
@@ -93,10 +95,12 @@ struct bluesleep_info {

 /* work function */
 static void bluesleep_sleep_work(struct work_struct *work);
+static void bluesleep_uart_awake_work(struct work_struct *work);
 static void bluesleep_ext_wake_set_wq(struct work_struct *work);

 /* work queue */
 DECLARE_DELAYED_WORK(sleep_workqueue, bluesleep_sleep_work);
+DECLARE_DELAYED_WORK(uart_awake_workqueue, bluesleep_uart_awake_work);
 DECLARE_DELAYED_WORK(tx_timer_expired_workqueue, bluesleep_ext_wake_set_wq);

 /* Macros for handling sleep work */
@@ -104,6 +108,7 @@ DECLARE_DELAYED_WORK(tx_timer_expired_workqueue, bluesleep_ext_wake_set_wq);
 #define bluesleep_tx_busy()     schedule_delayed_work(&sleep_workqueue, 0)
 #define bluesleep_rx_idle()     schedule_delayed_work(&sleep_workqueue, 0)
 #define bluesleep_tx_idle()     schedule_delayed_work(&sleep_workqueue, 0)
+#define bluesleep_uart_work()     schedule_delayed_work(&uart_awake_workqueue, 0)

 #define bluesleep_tx_timer_expired()     schedule_delayed_work(&tx_timer_expired_workqueue, 0)

@@ -117,12 +122,7 @@ DECLARE_DELAYED_WORK(tx_timer_expired_workqueue, bluesleep_ext_wake_set_wq);
 #define BT_EXT_WAKE	0x08
 #define BT_SUSPEND	0x10

-#if BT_BLUEDROID_SUPPORT
-static bool has_lpm_enabled;
-#else
-/* global pointer to a single hci device. */
-static struct hci_dev *bluesleep_hdev;
-#endif
+static bool bt_enabled;

 static struct platform_device *bluesleep_uart_dev;
 static struct bluesleep_info *bsi;
@@ -130,10 +130,6 @@ static struct bluesleep_info *bsi;
 /*
  * Local function prototypes
  */
-#if !BT_BLUEDROID_SUPPORT
-static int bluesleep_hci_event(struct notifier_block *this,
-			unsigned long event, void *data);
-#endif

 /*
  * Global variables
@@ -151,40 +147,75 @@ static DEFINE_TIMER(tx_timer, bluesleep_tx_timer_expire, 0, 0);
 /** Lock for state transitions */
 struct mutex bluesleep_mutex;

-#if !BT_BLUEDROID_SUPPORT
-/** Notifier block for HCI events */
-struct notifier_block hci_event_nblock = {
-	.notifier_call = bluesleep_hci_event,
-};
-#endif
-
 struct proc_dir_entry *bluetooth_dir, *sleep_dir;

 /*
  * Local functions
  */
-static void hsuart_power(int on)
+
+static int bluesleep_get_uart_state(void)
 {
-	if (test_bit(BT_SUSPEND, &flags) && !on) {
-		BT_DBG("hsuart_power OFF- it's suspend state. so return.");
-		return;
-	}
+	int state = 0;
+
+	state = msm_hs_get_clock_state(bsi->uport);
+	return state;
+}
+
+static void bluesleep_uart_awake_work(struct work_struct *work)
+{
+	int clk_state;

 	if (!bsi->uport) {
 		BT_DBG("hsuart_power called. But uport is null");
 		return;
 	}

-	if (on) {
-		BT_DBG("hsuart_power on");
+	clk_state = bluesleep_get_uart_state();
+	if (clk_state == MSM_HS_CLK_OFF) {
+		BT_DBG("bluesleep_uart_awake_work : hsuart_power on");
 		msm_hs_request_clock_on(bsi->uport);
 		msm_hs_set_mctrl(bsi->uport, TIOCM_RTS);
-	} else {
-		BT_DBG("hsuart_power off");
-		msm_hs_set_mctrl(bsi->uport, 0);
-		msm_hs_request_clock_off(bsi->uport);
+	}else if(clk_state == MSM_HS_CLK_REQUEST_OFF){
+		bluesleep_uart_work();
 	}
 }
+static void hsuart_power(int on)
+{
+    int clk_state;
+
+    if (test_bit(BT_SUSPEND, &flags)) {
+        BT_DBG("it's suspend state. waiting for resume.");
+        return;
+    }
+
+    if (!bsi->uport) {
+        BT_DBG("hsuart_power called. But uport is null");
+        return;
+    }
+
+    if (on) {
+        if(test_bit(BT_TXDATA, &flags)) {
+            BT_DBG("hsuart_power on");
+            msm_hs_request_clock_on(bsi->uport);
+            msm_hs_set_mctrl(bsi->uport, TIOCM_RTS);
+            return;
+		}
+
+        clk_state = bluesleep_get_uart_state();
+        if(clk_state == MSM_HS_CLK_REQUEST_OFF) {
+            BT_DBG("hsuart_power wait");
+            bluesleep_uart_work();
+        } else {
+            BT_DBG("hsuart_power on");
+            msm_hs_request_clock_on(bsi->uport);
+            msm_hs_set_mctrl(bsi->uport, TIOCM_RTS);
+        }
+    } else {
+        BT_DBG("hsuart_power off");
+        msm_hs_set_mctrl(bsi->uport, 0);
+        msm_hs_request_clock_off(bsi->uport);
+    }
+}

 /**
  * @return 1 if the Host can go to sleep, 0 otherwise.
@@ -198,26 +229,30 @@ int bluesleep_can_sleep(void)

 void bluesleep_sleep_wakeup(void)
 {
-	if (test_bit(BT_ASLEEP, &flags)) {
-		BT_DBG("waking up...");
-		/*Activating UART */
-		hsuart_power(1);
-		wake_lock(&bsi->wake_lock);
-		/* Start the timer */
-		mod_timer(&tx_timer, jiffies + (TX_TIMER_INTERVAL * HZ));
-		if (bsi->has_ext_wake == 1) {
-			int ret;
-			ret = ice_gpiox_set(bsi->ext_wake, 1);
-			if (ret)
-			{
-				BT_ERR("(bluesleep_sleep_wakeup) failed to set ext_wake 1.");
-				ret = ice_gpiox_set(bsi->ext_wake, 1);
-				BT_ERR("ret = %d", ret);
-			}
-		}
-		set_bit(BT_EXT_WAKE, &flags);
-		clear_bit(BT_ASLEEP, &flags);
-	}
+    if (test_bit(BT_ASLEEP, &flags)) {
+        BT_DBG("waking up...");
+        /*Activating UART */
+        hsuart_power(1);
+        wake_lock(&bsi->wake_lock);
+        /* Start the timer */
+        mod_timer(&tx_timer, jiffies + (TX_TIMER_INTERVAL * HZ));
+        if (bsi->has_ext_wake == 1) {
+            int ret;
+            ret = ice_gpiox_set(bsi->ext_wake, 1);
+            if (ret)
+            {
+                BT_ERR("(bluesleep_sleep_wakeup) failed to set ext_wake 1.");
+                ret = ice_gpiox_set(bsi->ext_wake, 1);
+                BT_ERR("ret = %d", ret);
+            }
+        }
+        set_bit(BT_EXT_WAKE, &flags);
+        clear_bit(BT_ASLEEP, &flags);
+    }
+    else {
+        BT_DBG("bluesleep_sleep_wakeup : already wake up, so start timer...");
+        mod_timer(&tx_timer, jiffies + (TX_TIMER_INTERVAL * HZ));
+    }
 }

 static void bluesleep_ext_wake_set_wq(struct work_struct *work)
@@ -230,12 +265,12 @@ static void bluesleep_ext_wake_set_wq(struct work_struct *work)

 static void bluesleep_tx_data_wakeup(void)
 {
-	if (test_bit(BT_ASLEEP, &flags)) {
-		BT_DBG("waking up from BT Write...");
+    if (test_bit(BT_ASLEEP, &flags)) {
+        BT_DBG("waking up from BT Write...");

-		wake_lock(&bsi->wake_lock);
-		/* Start the timer */
-		mod_timer(&tx_timer, jiffies + (TX_TIMER_INTERVAL * HZ));
+        wake_lock(&bsi->wake_lock);
+        /* Start the timer */
+        mod_timer(&tx_timer, jiffies + (TX_TIMER_INTERVAL * HZ));
         if (bsi->has_ext_wake == 1) {
             int ret;
             ret = ice_gpiox_set(bsi->ext_wake, 1);
@@ -252,9 +287,13 @@ static void bluesleep_tx_data_wakeup(void)
                 }
             }
         }
-		set_bit(BT_EXT_WAKE, &flags);
-		clear_bit(BT_ASLEEP, &flags);
-	}
+        set_bit(BT_EXT_WAKE, &flags);
+        clear_bit(BT_ASLEEP, &flags);
+    }
+    else {
+        BT_DBG("bluesleep_tx_data_wakeup : already wake up, so start timer...");
+        mod_timer(&tx_timer, jiffies + (TX_TIMER_INTERVAL * HZ));
+    }
 }


@@ -266,60 +305,70 @@ static void bluesleep_tx_data_wakeup(void)
  */
 static void bluesleep_sleep_work(struct work_struct *work)
 {
-	if (mutex_is_locked(&bluesleep_mutex))
-		BT_DBG("Wait for mutex unlock in bluesleep_sleep_work");
+    if (mutex_is_locked(&bluesleep_mutex))
+        BT_DBG("Wait for mutex unlock in bluesleep_sleep_work");

-	mutex_lock(&bluesleep_mutex);
-
-	if (bluesleep_can_sleep()) {
-		/* already asleep, this is an error case */
-		if (test_bit(BT_ASLEEP, &flags)) {
-			BT_DBG("already asleep");
-			mutex_unlock(&bluesleep_mutex);
-			return;
-		}
+    mutex_lock(&bluesleep_mutex);

-		if (msm_hs_tx_empty(bsi->uport)) {
-			if (test_bit(BT_TXDATA, &flags)) {
-				BT_DBG("TXDATA remained. Wait until timer expires.");
+    if (bluesleep_can_sleep()) {
+        /* already asleep, this is an error case */
+        if (test_bit(BT_ASLEEP, &flags)) {
+            BT_DBG("already asleep");
+            mutex_unlock(&bluesleep_mutex);
+            return;
+        }

-				mod_timer(&tx_timer, jiffies + (TX_TIMER_INTERVAL * HZ));
-				mutex_unlock(&bluesleep_mutex);
-				return;
-			}
+        if (msm_hs_tx_empty(bsi->uport)) {
+            if (test_bit(BT_TXDATA, &flags)) {
+                BT_DBG("TXDATA remained. Wait until timer expires.");

-			BT_DBG("going to sleep...");
+                mod_timer(&tx_timer, jiffies + (TX_TIMER_INTERVAL * HZ));
+                mutex_unlock(&bluesleep_mutex);
+                return;
+            }

-			set_bit(BT_ASLEEP, &flags);
-			/*Deactivating UART */
-			hsuart_power(0);
+            BT_DBG("going to sleep...");

-			/*Deactivating UART */
-			/* UART clk is not turned off immediately. Release
-			 * wakelock after 125 ms.
-			 */
-			wake_lock_timeout(&bsi->wake_lock, HZ / 8);
-		} else {
-			BT_DBG("host can enter sleep but some tx remained.");
+            set_bit(BT_ASLEEP, &flags);
+            /*Deactivating UART */
+            hsuart_power(0);

-			mod_timer(&tx_timer, jiffies + (TX_TIMER_INTERVAL * HZ));
-			mutex_unlock(&bluesleep_mutex);
-			return;
-		}
-	} else if (!test_bit(BT_EXT_WAKE, &flags)
-			&& !test_bit(BT_ASLEEP, &flags)) {
-		mod_timer(&tx_timer, jiffies + (TX_TIMER_INTERVAL * HZ));
-		if (bsi->has_ext_wake == 1) {
-			int ret;
-			ret = ice_gpiox_set(bsi->ext_wake, 1);
-			if (ret)
-				BT_ERR("(bluesleep_sleep_work) failed to set ext_wake 1.");
-		}
-		set_bit(BT_EXT_WAKE, &flags);
-	} else {
-		bluesleep_sleep_wakeup();
-	}
-	mutex_unlock(&bluesleep_mutex);
+            /* Moved from Timer expired */
+            if (bsi->has_ext_wake == 1) {
+                int ret;
+                ret = ice_gpiox_set(bsi->ext_wake, 0);
+                if (ret)
+                    BT_ERR("(bluesleep_sleep_work) failed to set ext_wake.");
+            }
+            clear_bit(BT_EXT_WAKE, &flags);
+
+            /*Deactivating UART */
+            /* UART clk is not turned off immediately. Release
+            * wakelock after 125 ms.
+            */
+            wake_lock_timeout(&bsi->wake_lock, HZ / 8);
+        } else {
+            BT_DBG("host can enter sleep but some tx remained.");
+
+            mod_timer(&tx_timer, jiffies + (TX_TIMER_INTERVAL * HZ));
+            mutex_unlock(&bluesleep_mutex);
+            return;
+        }
+    } else if (!test_bit(BT_EXT_WAKE, &flags)
+            && !test_bit(BT_ASLEEP, &flags)) {
+        BT_DBG("host_wake high and BT_EXT_WAKE & BT_ASLEEP already freed.");
+        mod_timer(&tx_timer, jiffies + (TX_TIMER_INTERVAL * HZ));
+        if (bsi->has_ext_wake == 1) {
+            int ret;
+            ret = ice_gpiox_set(bsi->ext_wake, 1);
+            if (ret)
+                BT_ERR("(bluesleep_sleep_work) failed to set ext_wake 1.");
+        }
+        set_bit(BT_EXT_WAKE, &flags);
+    } else {
+        bluesleep_sleep_wakeup();
+    }
+    mutex_unlock(&bluesleep_mutex);
 }

 /**
@@ -363,14 +412,19 @@ static void bluesleep_outgoing_data(void)

 	BT_DBG("bluesleep_outgoing_data.");

-	/* if the tx side is sleeping... */
-	if (!test_bit(BT_EXT_WAKE, &flags) || test_bit(BT_ASLEEP, &flags)) {
-		BT_DBG("tx was sleeping");
+	if (!test_bit(BT_EXT_WAKE, &flags))
+		BT_DBG("BT_EXT_WAKE freed");

-		hsuart_power(1);
+	if (!test_bit(BT_ASLEEP, &flags))
+		BT_DBG("BT_ASLEEP freed");

-		bluesleep_tx_data_wakeup();
-	}
+	/*
+	** Uart Clk should be enabled promptly
+	** before bluedroid write TX data.
+	*/
+	hsuart_power(1);
+
+	bluesleep_tx_data_wakeup();

 	mutex_unlock(&bluesleep_mutex);
 }
@@ -397,13 +451,11 @@ static void bluesleep_start(void)
 			BT_ERR("(bluesleep_start) failed to set ext_wake 1.");
 	}
 	set_bit(BT_EXT_WAKE, &flags);
-#if BT_ENABLE_IRQ_WAKE
 	retval = enable_irq_wake(bsi->host_wake_irq);
 	if (retval < 0) {
 		BT_ERR("Couldn't enable BT_HOST_WAKE as wakeup interrupt");
 		goto fail;
 	}
-#endif
 	set_bit(BT_PROTO, &flags);
 	wake_lock(&bsi->wake_lock);
 	return;
@@ -415,25 +467,23 @@ fail:

 static void bluesleep_abnormal_stop(void)
 {
-	BT_ERR("bluesleep_abnormal_stop");
+    BT_ERR("bluesleep_abnormal_stop");

-	if (!test_bit(BT_PROTO, &flags)) {
-		BT_ERR("(bluesleep_abnormal_stop) proto is not set. Failed to stop bluesleep");
-		bsi->uport = NULL;
-		return;
-	}
+    if (!test_bit(BT_PROTO, &flags)) {
+        BT_ERR("(bluesleep_abnormal_stop) proto is not set. Failed to stop bluesleep");
+        bsi->uport = NULL;
+        return;
+    }

-	del_timer(&tx_timer);
-	clear_bit(BT_PROTO, &flags);
+    del_timer(&tx_timer);
+    clear_bit(BT_PROTO, &flags);

-#if BT_ENABLE_IRQ_WAKE
-	if (disable_irq_wake(bsi->host_wake_irq))
-		BT_ERR("Couldn't disable hostwake IRQ wakeup mode\n");
-#endif
-	wake_lock_timeout(&bsi->wake_lock, HZ / 8);
+    if (disable_irq_wake(bsi->host_wake_irq))
+        BT_ERR("Couldn't disable hostwake IRQ wakeup mode\n");
+    wake_lock_timeout(&bsi->wake_lock, HZ / 8);

-	clear_bit(BT_TXDATA, &flags);
-	bsi->uport = NULL;
+    clear_bit(BT_TXDATA, &flags);
+    bsi->uport = NULL;
 }

 /**
@@ -462,16 +512,13 @@ static void bluesleep_stop(void)
 		hsuart_power(1);
 	}

-#if BT_ENABLE_IRQ_WAKE
 	if (disable_irq_wake(bsi->host_wake_irq))
 		BT_ERR("Couldn't disable hostwake IRQ wakeup mode\n");
-#endif
 	wake_lock_timeout(&bsi->wake_lock, HZ / 8);

 	bsi->uport = NULL;
 }

-#if BT_BLUEDROID_SUPPORT
 struct uart_port *bluesleep_get_uart_port(void)
 {
 	struct uart_port *uport = NULL;
@@ -487,7 +534,7 @@ static int bluesleep_read_proc_lpm(char *page, char **start, off_t offset,
 					int count, int *eof, void *data)
 {
 	*eof = 1;
-	return snprintf(page, count, "lpm: %u\n", has_lpm_enabled?1:0 );
+	return snprintf(page, count, "lpm: %u\n", bt_enabled?1:0 );
 }

 static int bluesleep_write_proc_lpm(struct file *file, const char *buffer,
@@ -495,36 +542,36 @@ static int bluesleep_write_proc_lpm(struct file *file, const char *buffer,
 {
 	char b;

-	if (count < 1)
-		return -EINVAL;
-
-	if (copy_from_user(&b, buffer, 1))
-		return -EFAULT;
-
-	if (b == '0') {
-		BT_ERR("(bluesleep_write_proc_lpm) Unreg HCI notifier.");
-		/* HCI_DEV_UNREG */
-		bluesleep_stop();
-		has_lpm_enabled = false;
-		//bsi->uport = NULL;
-	} else if (b == '1') {
-		BT_ERR("(bluesleep_write_proc_lpm) Reg HCI notifier.");
-		/* HCI_DEV_REG */
-		if (!has_lpm_enabled) {
-			has_lpm_enabled = true;
-			bsi->uport = bluesleep_get_uart_port();
-			/* if bluetooth started, start bluesleep*/
-			bluesleep_start();
-		}
-	} else if (b == '2') {
-		BT_ERR("(bluesleep_write_proc_lpm) don`t control ext_wake & uart clk");
-		if(has_lpm_enabled) {
-			has_lpm_enabled = false;
-			bluesleep_abnormal_stop();
-		}
-	}
+    if (count < 1)
+        return -EINVAL;
+
+    if (copy_from_user(&b, buffer, 1))
+        return -EFAULT;
+
+    if (b == '0') {
+        BT_ERR("(bluesleep_write_proc_lpm) Unreg HCI notifier.");
+        /* HCI_DEV_UNREG */
+        bluesleep_stop();
+        bt_enabled = false;
+        //bsi->uport = NULL;
+    } else if (b == '1') {
+        BT_ERR("(bluesleep_write_proc_lpm) Reg HCI notifier.");
+        /* HCI_DEV_REG */
+        if (!bt_enabled) {
+            bt_enabled = true;
+            bsi->uport = bluesleep_get_uart_port();
+            /* if bluetooth started, start bluesleep*/
+            bluesleep_start();
+        }
+    } else if (b == '2') {
+        BT_ERR("(bluesleep_write_proc_lpm) don`t control ext_wake & uart clk");
+        if(bt_enabled) {
+            bt_enabled = false;
+            bluesleep_abnormal_stop();
+        }
+    }

-	return count;
+    return count;
 }

 static int bluesleep_read_proc_btwrite(char *page, char **start, off_t offset,
@@ -551,49 +598,6 @@ static int bluesleep_write_proc_btwrite(struct file *file, const char *buffer,

 	return count;
 }
-#else
-/**
- * Handles HCI device events.
- * @param this Not used.
- * @param event The event that occurred.
- * @param data The HCI device associated with the event.
- * @return <code>NOTIFY_DONE</code>.
- */
-static int bluesleep_hci_event(struct notifier_block *this,
-				unsigned long event, void *data)
-{
-	struct hci_dev *hdev = (struct hci_dev *) data;
-	struct hci_uart *hu;
-	struct uart_state *state;
-
-	if (!hdev)
-		return NOTIFY_DONE;
-
-	switch (event) {
-	case HCI_DEV_REG:
-		if (!bluesleep_hdev) {
-			bluesleep_hdev = hdev;
-			hu  = (struct hci_uart *) hdev->driver_data;
-			state = (struct uart_state *) hu->tty->driver_data;
-			bsi->uport = state->uart_port;
-			/* if bluetooth started, start bluesleep*/
-			bluesleep_start();
-		}
-		break;
-	case HCI_DEV_UNREG:
-		bluesleep_stop();
-		bluesleep_hdev = NULL;
-		bsi->uport = NULL;
-		/* if bluetooth stopped, stop bluesleep also */
-		break;
-	case HCI_DEV_WRITE:
-		bluesleep_outgoing_data();
-		break;
-	}
-
-	return NOTIFY_DONE;
-}
-#endif

 /**
  * Handles transmission timer expiration.
@@ -604,11 +608,7 @@ static void bluesleep_tx_timer_expire(unsigned long data)
 	/* were we silent during the last timeout? */
 	if (!test_bit(BT_TXDATA, &flags)) {
 		BT_DBG("Tx has been idle");
-		if (bsi->has_ext_wake == 1)
-		{
-			bluesleep_tx_timer_expired();
-		}
-		clear_bit(BT_EXT_WAKE, &flags);
+
 		bluesleep_tx_idle();
 	} else {
 		BT_DBG("Tx data during last period");
@@ -860,7 +860,7 @@ static int bluesleep_probe(struct platform_device *pdev)

 		/* configure ext_wake as output mode*/
 		/* uses FPGA
-		ret = gpio_direction_output(bsi->ext_wake, 1);
+		ret = gpio_direction_output(bsi->ext_wake, 0);
 		if (ret < 0) {
 			BT_ERR("gpio-keys: failed to configure output direction for GPIO %d, error %d",
 				  bsi->ext_wake, ret);
@@ -933,24 +933,24 @@ static int bluesleep_remove(struct platform_device *pdev)
 static int bluesleep_resume(struct platform_device *pdev)
 {
 	if (test_bit(BT_SUSPEND, &flags)) {
-		if (!has_lpm_enabled) {
+		if (!bt_enabled) {
 			gpio_tlmm_config(GPIO_CFG(bsi->host_wake, 0, GPIO_CFG_INPUT,
 						GPIO_CFG_NO_PULL, GPIO_CFG_16MA), GPIO_CFG_ENABLE);
 		}

+		clear_bit(BT_SUSPEND, &flags);
 		if ((bsi->uport != NULL) &&
 			(gpio_get_value(bsi->host_wake) == bsi->irq_polarity)) {
 				BT_DBG("bluesleep resume form BT event...");
 				hsuart_power(1);
 		}
-		clear_bit(BT_SUSPEND, &flags);
 	}
 	return 0;
 }

 static int bluesleep_suspend(struct platform_device *pdev, pm_message_t state)
 {
-	if (!has_lpm_enabled) {
+	if (!bt_enabled) {
 		gpio_tlmm_config(GPIO_CFG(bsi->host_wake, 0, GPIO_CFG_INPUT,
 						GPIO_CFG_PULL_DOWN, GPIO_CFG_16MA), GPIO_CFG_ENABLE);
 	}
@@ -976,141 +976,126 @@ static struct platform_driver bluesleep_driver = {
  */
 static int __init bluesleep_init(void)
 {
-	int retval;
-	struct proc_dir_entry *ent;
-
-	BT_INFO("BlueSleep Mode Driver Ver %s", VERSION);
-
-#if BT_BLUEDROID_SUPPORT
-	has_lpm_enabled = false;
-#endif
-
-	retval = platform_driver_register(&bluesleep_driver);
-
-	if (retval)
-		return retval;
-
-	if (bsi == NULL)
-	{
-		BT_ERR("bluesleep_init failed. bsi is NULL!!");
-		return -1;
-	}
-
-#if !BT_BLUEDROID_SUPPORT
-	bluesleep_hdev = NULL;
-#endif
-
-	bluetooth_dir = proc_mkdir("bluetooth", NULL);
-	if (bluetooth_dir == NULL) {
-		BT_ERR("Unable to create /proc/bluetooth directory");
-		return -ENOMEM;
-	}
-
-	sleep_dir = proc_mkdir("sleep", bluetooth_dir);
-	if (sleep_dir == NULL) {
-		BT_ERR("Unable to create /proc/%s directory", PROC_DIR);
-		return -ENOMEM;
-	}
-
-	/* Creating read/write "btwake" entry */
-	ent = create_proc_entry("btwake", 0, sleep_dir);
-	if (ent == NULL) {
-		BT_ERR("Unable to create /proc/%s/btwake entry", PROC_DIR);
-		retval = -ENOMEM;
-		goto fail;
-	}
-	ent->read_proc = bluepower_read_proc_btwake;
-	ent->write_proc = bluepower_write_proc_btwake;
-
-	/* read only proc entries */
-	if (create_proc_read_entry("hostwake", 0, sleep_dir,
-				bluepower_read_proc_hostwake, NULL) == NULL) {
-		BT_ERR("Unable to create /proc/%s/hostwake entry", PROC_DIR);
-		retval = -ENOMEM;
-		goto fail;
-	}
-
-	/* read/write proc entries */
-	ent = create_proc_entry("proto", 0, sleep_dir);
-	if (ent == NULL) {
-		BT_ERR("Unable to create /proc/%s/proto entry", PROC_DIR);
-		retval = -ENOMEM;
-		goto fail;
-	}
-	ent->read_proc = bluesleep_read_proc_proto;
-	ent->write_proc = bluesleep_write_proc_proto;
-
-	/* read only proc entries */
-	if (create_proc_read_entry("asleep", 0,
-			sleep_dir, bluesleep_read_proc_asleep, NULL) == NULL) {
-		BT_ERR("Unable to create /proc/%s/asleep entry", PROC_DIR);
-		retval = -ENOMEM;
-		goto fail;
-	}
-
-#if BT_BLUEDROID_SUPPORT
-	/* read/write proc entries */
-	ent = create_proc_entry("lpm", 0, sleep_dir);
-	if (ent == NULL) {
-		BT_ERR("Unable to create /proc/%s/lpm entry", PROC_DIR);
-		retval = -ENOMEM;
-		goto fail;
-	}
-	ent->read_proc = bluesleep_read_proc_lpm;
-	ent->write_proc = bluesleep_write_proc_lpm;
-
-	/* read/write proc entries */
-	ent = create_proc_entry("btwrite", 0, sleep_dir);
-	if (ent == NULL) {
-		BT_ERR("Unable to create /proc/%s/btwrite entry", PROC_DIR);
-		retval = -ENOMEM;
-		goto fail;
-	}
-	ent->read_proc = bluesleep_read_proc_btwrite;
-	ent->write_proc = bluesleep_write_proc_btwrite;
-#endif
-
-	flags = 0; /* clear all status bits */
-
-	/* Initialize spinlock. */
-	mutex_init(&bluesleep_mutex);
-
-	/* Initialize timer */
-	init_timer(&tx_timer);
-	tx_timer.function = bluesleep_tx_timer_expire;
-	tx_timer.data = 0;
-
-	/* initialize host wake tasklet */
-	tasklet_init(&hostwake_task, bluesleep_hostwake_task, 0);
-
-	/* assert bt wake */
-	/* block code for FPGA to be set-up
-	if (bsi->has_ext_wake == 1) {
-		int ret;
-		ret = ice_gpiox_set(bsi->ext_wake, 1);
-		if (ret)
-			BT_ERR("(bluesleep_init) failed to set ext_wake 1.");
-	}
-	*/
-	set_bit(BT_EXT_WAKE, &flags);
-#if !BT_BLUEDROID_SUPPORT
-	hci_register_notifier(&hci_event_nblock);
-#endif
-
-	return 0;
+    int retval;
+    struct proc_dir_entry *ent;
+
+    BT_INFO("BlueSleep Mode Driver Ver %s", VERSION);
+    bt_enabled = false;
+    retval = platform_driver_register(&bluesleep_driver);
+
+    if (retval)
+        return retval;
+
+    if (bsi == NULL)
+    {
+        BT_ERR("bluesleep_init failed. bsi is NULL!!");
+        return -1;
+    }
+
+    bluetooth_dir = proc_mkdir("bluetooth", NULL);
+    if (bluetooth_dir == NULL) {
+        BT_ERR("Unable to create /proc/bluetooth directory");
+        return -ENOMEM;
+    }
+
+    sleep_dir = proc_mkdir("sleep", bluetooth_dir);
+    if (sleep_dir == NULL) {
+        BT_ERR("Unable to create /proc/%s directory", PROC_DIR);
+        return -ENOMEM;
+    }
+
+    /* Creating read/write "btwake" entry */
+    ent = create_proc_entry("btwake", 0, sleep_dir);
+    if (ent == NULL) {
+        BT_ERR("Unable to create /proc/%s/btwake entry", PROC_DIR);
+        retval = -ENOMEM;
+        goto fail;
+    }
+    ent->read_proc = bluepower_read_proc_btwake;
+    ent->write_proc = bluepower_write_proc_btwake;
+
+    /* read only proc entries */
+    if (create_proc_read_entry("hostwake", 0, sleep_dir,
+                bluepower_read_proc_hostwake, NULL) == NULL) {
+        BT_ERR("Unable to create /proc/%s/hostwake entry", PROC_DIR);
+        retval = -ENOMEM;
+        goto fail;
+    }
+
+    /* read/write proc entries */
+    ent = create_proc_entry("proto", 0, sleep_dir);
+    if (ent == NULL) {
+        BT_ERR("Unable to create /proc/%s/proto entry", PROC_DIR);
+        retval = -ENOMEM;
+        goto fail;
+    }
+    ent->read_proc = bluesleep_read_proc_proto;
+    ent->write_proc = bluesleep_write_proc_proto;
+
+    /* read only proc entries */
+    if (create_proc_read_entry("asleep", 0,
+            sleep_dir, bluesleep_read_proc_asleep, NULL) == NULL) {
+        BT_ERR("Unable to create /proc/%s/asleep entry", PROC_DIR);
+        retval = -ENOMEM;
+        goto fail;
+    }
+
+    /* read/write proc entries */
+    ent = create_proc_entry("lpm", 0, sleep_dir);
+    if (ent == NULL) {
+        BT_ERR("Unable to create /proc/%s/lpm entry", PROC_DIR);
+        retval = -ENOMEM;
+        goto fail;
+    }
+    ent->read_proc = bluesleep_read_proc_lpm;
+    ent->write_proc = bluesleep_write_proc_lpm;
+
+    /* read/write proc entries */
+    ent = create_proc_entry("btwrite", 0, sleep_dir);
+    if (ent == NULL) {
+        BT_ERR("Unable to create /proc/%s/btwrite entry", PROC_DIR);
+        retval = -ENOMEM;
+        goto fail;
+    }
+    ent->read_proc = bluesleep_read_proc_btwrite;
+    ent->write_proc = bluesleep_write_proc_btwrite;
+
+    flags = 0; /* clear all status bits */
+
+    /* Initialize spinlock. */
+    mutex_init(&bluesleep_mutex);
+
+    /* Initialize timer */
+    init_timer(&tx_timer);
+    tx_timer.function = bluesleep_tx_timer_expire;
+    tx_timer.data = 0;
+
+    /* initialize host wake tasklet */
+    tasklet_init(&hostwake_task, bluesleep_hostwake_task, 0);
+
+    /* assert bt wake */
+    /* block code for FPGA to be set-up
+    if (bsi->has_ext_wake == 1) {
+        int ret;
+        ret = ice_gpiox_set(bsi->ext_wake, 1);
+        if (ret)
+            BT_ERR("(bluesleep_init) failed to set ext_wake 1.");
+    }
+    */
+    set_bit(BT_EXT_WAKE, &flags);
+
+    return 0;

 fail:
-#if BT_BLUEDROID_SUPPORT
-	remove_proc_entry("btwrite", sleep_dir);
-	remove_proc_entry("lpm", sleep_dir);
-#endif
-	remove_proc_entry("asleep", sleep_dir);
-	remove_proc_entry("proto", sleep_dir);
-	remove_proc_entry("hostwake", sleep_dir);
-	remove_proc_entry("btwake", sleep_dir);
-	remove_proc_entry("sleep", bluetooth_dir);
-	remove_proc_entry("bluetooth", 0);
-	return retval;
+    remove_proc_entry("btwrite", sleep_dir);
+    remove_proc_entry("lpm", sleep_dir);
+    remove_proc_entry("asleep", sleep_dir);
+    remove_proc_entry("proto", sleep_dir);
+    remove_proc_entry("hostwake", sleep_dir);
+    remove_proc_entry("btwake", sleep_dir);
+    remove_proc_entry("sleep", bluetooth_dir);
+    remove_proc_entry("bluetooth", 0);
+    return retval;
 }

 /**
@@ -1138,15 +1123,10 @@ static void __exit bluesleep_exit(void)
 			hsuart_power(1);
 	}

-#if !BT_BLUEDROID_SUPPORT
-	hci_unregister_notifier(&hci_event_nblock);
-#endif
 	platform_driver_unregister(&bluesleep_driver);

-#if BT_BLUEDROID_SUPPORT
 	remove_proc_entry("btwrite", sleep_dir);
 	remove_proc_entry("lpm", sleep_dir);
-#endif
 	remove_proc_entry("asleep", sleep_dir);
 	remove_proc_entry("proto", sleep_dir);
 	remove_proc_entry("hostwake", sleep_dir);
@@ -1164,3 +1144,4 @@ MODULE_DESCRIPTION("Bluetooth Sleep Mode Driver ver %s " VERSION);
 #ifdef MODULE_LICENSE
 MODULE_LICENSE("GPL");
 #endif
+
---
blah.