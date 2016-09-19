#define MSM_LIMIT			"msm_limiter"
#define LIMITER_ENABLED			1
#define DEFAULT_SUSPEND_DEFER_TIME	10
#define DEFAULT_SUSPEND_FREQUENCY	810000
#if defined(CONFIG_ARCH_APQ8084)
#define DEFAULT_RESUME_FREQUENCY	1890000
#else
#define DEFAULT_RESUME_FREQUENCY	1890000
#endif
#define DEFAULT_MIN_FREQUENCY		384000

static struct cpu_limit {
	unsigned int limiter_enabled;
	uint32_t suspend_max_freq;
	uint32_t resume_max_freq[4];
	uint32_t suspend_min_freq[4];
	unsigned int suspended;
	unsigned int suspend_defer_time;
	struct delayed_work suspend_work;
	struct work_struct resume_work;
	struct mutex resume_suspend_mutex;
	struct mutex msm_limiter_mutex[4];
	struct notifier_block notif;
} limit = {
	.limiter_enabled = LIMITER_ENABLED,
	.suspend_max_freq = DEFAULT_SUSPEND_FREQUENCY,
	.resume_max_freq[0] = DEFAULT_RESUME_FREQUENCY,
	.resume_max_freq[1] = DEFAULT_RESUME_FREQUENCY,
	.resume_max_freq[2] = DEFAULT_RESUME_FREQUENCY,
	.resume_max_freq[3] = DEFAULT_RESUME_FREQUENCY,
	.suspend_min_freq[0] = DEFAULT_MIN_FREQUENCY,
	.suspend_min_freq[1] = DEFAULT_MIN_FREQUENCY,
	.suspend_min_freq[2] = DEFAULT_MIN_FREQUENCY,
	.suspend_min_freq[3] = DEFAULT_MIN_FREQUENCY,
	.suspend_defer_time = DEFAULT_SUSPEND_DEFER_TIME,
};

int cpufreq_set_gov(char *target_gov, unsigned int cpu);
char *cpufreq_get_gov(unsigned int cpu);
int cpufreq_set_freq(unsigned int max_freq, unsigned int min_freq,
			unsigned int cpu);
int cpufreq_get_max(unsigned int cpu);
int cpufreq_get_min(unsigned int cpu);
