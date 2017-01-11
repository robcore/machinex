int iterate_fd(struct files_struct *files, unsigned n,
		int (*f)(const void *, struct file *, unsigned),
		const void *p)
{
	struct fdtable *fdt;
	struct file *file;
	int res = 0;
	if (!files)
		return 0;
	spin_lock(&files->file_lock);
	fdt = files_fdtable(files);
	while (!res && n < fdt->max_fds) {
		file = rcu_dereference_check_fdtable(files, fdt->fd[n++]);
		if (file)
			res = f(p, file, n);
	}
	spin_unlock(&files->file_lock);
	return res;
}
EXPORT_SYMBOL(iterate_fd);