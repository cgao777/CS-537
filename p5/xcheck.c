#include<sys/types.h>
#include<sys/stat.h>
#include<stdio.h>
#include<stdlib.h>
#include<sys/mman.h>
#include<unistd.h>
#include<fcntl.h>
#include<unistd.h>
#include<string.h>
#define stat xv6_stat
#define dirent xv6_dirent
#include "fs.h"
#include "types.h"
#include "stat.h"
#undef stat
#undef dirent

struct superblock *sb;
struct dinode *dip;
void *img_ptr;
char *bitmap;
int *inodes;
int *found;
int inodes_count;
int found_count;
int *reference_count;
int traverse(int d, int* list) {
	for (int i = 0; i < found_count; i++) {
		if (d == list[i])
			return 1;
	}
	return 0;
}

int check_dir(struct dinode *d, int flag) {

	int count_dot = 0;
	int count_dot_dot = 0;

	for (int i = 0; i < NDIRECT; i++) {
		if (d->addrs[i] == 0)
			continue;
		struct xv6_dirent *dir = (struct xv6_dirent *)(img_ptr + d->addrs[i] * BSIZE);

		for (int j = 0; j < BSIZE / sizeof(struct xv6_dirent); j++) {
			if (flag == 1 && !strcmp(dir[j].name, ".")) {
				count_dot++;
				if (&dip[dir[j].inum] != d) {
					return 1;
				}
			}
			if ((flag == 0 || flag == 1) && !strcmp(dir[j].name, "..")) {
				count_dot_dot++;
				if (d == &dip[1] && &dip[dir[j].inum] != d)
					return 1;
			}
			if (dir[j].inum == 0)
				continue;

			if (flag == 2 && dip[dir[j].inum].type == 0)
				return 1;

			if (flag == 3) {
				if (!traverse(dir[j].inum, found) && strcmp(dir[j].name, "..")) {
					if (strcmp(dir[j].name, ".") || dir[j].inum == 1) {
						found[found_count] = dir[j].inum;
						found_count++;
						reference_count[dir[j].inum]++;
					}
				}
				else {
					if (dip[dir[j].inum].type == T_DIR && strcmp(dir[j].name, ".") && strcmp(dir[j].name, "..")) {
						fprintf(stderr, "ERROR: directory appears more than once in file system.\n");
						exit(1);
					}
					reference_count[dir[j].inum]++;
				}
			}
		}
	}
	if (flag == 1 && (count_dot < 1 || count_dot_dot < 1))
		return 1;
	return 0;
}

void check() {
	char bm[BSIZE] = { 0 };
	int mapcount;
	for (mapcount = 0; mapcount < (sb->ninodes / IPB + 4) / 8; mapcount++) {
		bm[mapcount] = -1;
	}
	for (int i = 0; i < (sb->ninodes / IPB + 4) % 8; i++) {
		bm[mapcount] += 1 << i;
	}

	for (int i = 1; i < sb->ninodes + 1; i++) {
		if (i == 1) {
			if (dip[i].type != T_DIR || check_dir(&dip[i], 0)) {
				fprintf(stderr, "ERROR: root directory does not exist.\n");
				exit(1);
			}
		}
		if (dip[i].type == 0)
			continue;
		if (!(dip[i].type == T_FILE || dip[i].type == T_DIR || dip[i].type == T_DEV)) {
			fprintf(stderr, "ERROR: bad inode.\n");
			exit(1);
		}
		for (int j = 0; j < NDIRECT; j++)
		{
			if (dip[i].addrs[j] == 0)
				continue;
			if (dip[i].addrs[j] < (sb->ninodes / IPB + 4) || dip[i].addrs[j] > sb->size) {
				fprintf(stderr, "ERROR: bad direct address in inode.\n");
				exit(1);
			}
			if (((bitmap[(dip[i].addrs[j]) / 8] & (1 << (dip[i].addrs[j]) % 8))) == 0) {
				fprintf(stderr, "ERROR: address used by inode but marked free in bitmap.\n");
				exit(1);
			}
			if (((bm[dip[i].addrs[j] / 8] & (1 << (dip[i].addrs[j] % 8)))) == 0) {
				bm[dip[i].addrs[j] / 8] += 1 << (dip[i].addrs[j] % 8);
			}
			else {
				fprintf(stderr, "ERROR: direct address used more than once.\n");
				exit(1);
			}
		}

		if (dip[i].addrs[NDIRECT] != 0) {
			if (dip[i].addrs[NDIRECT] < (sb->ninodes / IPB + 4) || dip[i].addrs[NDIRECT] > sb->size) {
				fprintf(stderr, "ERROR: bad indirect address in inode.\n");
				exit(1);
			}
			if (((bm[dip[i].addrs[NDIRECT] / 8] & (1 << (dip[i].addrs[NDIRECT] % 8)))) == 0) {
				bm[dip[i].addrs[NDIRECT] / 8] += 1 << (dip[i].addrs[NDIRECT] % 8);
			}
			else {
				fprintf(stderr, "ERROR: indirect address used more than once.\n");
				exit(1);
			}

			uint *indirect = (uint *)(img_ptr + dip[i].addrs[NDIRECT] * BSIZE);
			for (int j = 0; j < NINDIRECT; j++) {
				if (indirect[j] == 0)
					continue;
				if (indirect[j] < (sb->ninodes / IPB + 4) || indirect[j] > sb->size) {
					fprintf(stderr, "ERROR: bad indirect address in inode.\n");
					exit(1);
				}
				if (((bitmap[(indirect[j]) / 8] & (1 << (indirect[j]) % 8))) == 0) {
					fprintf(stderr, "ERROR: address used by inode but marked free in bitmap.\n");
					exit(1);
				}
				if (((bm[indirect[j] / 8] & (1 << (indirect[j] % 8)))) == 0) {
					bm[indirect[j] / 8] += 1 << (indirect[j] % 8);
				}
				else {
					fprintf(stderr, "ERROR: indirect address used more than once.\n");
					exit(1);
				}
			}
		}
		if (dip[i].type == T_DIR) {
			if (check_dir(&dip[i], 1)) {
				fprintf(stderr, "ERROR: directory not properly formatted.\n");
				exit(1);
			}
			if (check_dir(&dip[i], 2)) {
				fprintf(stderr, "ERROR: inode referred to in directory but marked free.\n");
				exit(1);
			}
			check_dir(&dip[i], 3);
		}
		inodes[inodes_count] = i;
		inodes_count++;
	}

	for (int i = 0; i < sb->size / 8; i++) {
		if (bm[i] != bitmap[i]) {
			fprintf(stderr, "ERROR: bitmap marks block in use but it is not in use.\n");
			exit(1);
		}
	}
	for (int i = 1; i < sb->ninodes + 1; i++) {
		if (dip[i].type == 0)
			continue;
		if (dip[i].type == T_FILE && dip[i].nlink != reference_count[i]) {
			fprintf(stderr, "ERROR: bad reference count for file.\n");
			exit(1);
		}
	}
}

void extra_credit() {
	// extra credit 2
	for (int i = 1; i < sb->ninodes + 1; i++) {
		if (dip[i].type == T_DIR) {
			int flag = 0;
			struct dinode *current = &dip[i];
			for (int j = 1; j < sb->ninodes + 1; j++) {
				for (int k = 0; k < NDIRECT; k++) {
					if (current->addrs[k] == 0)
						continue;
					struct xv6_dirent *dir = (struct xv6_dirent *)(img_ptr + current->addrs[k] * BSIZE);
					for (int l = 0; l < BSIZE / sizeof(struct xv6_dirent); l++) {
						if (!strcmp(dir[l].name, "..")) {
							if (dir[l].inum == 1)
								flag = 1;
							current = &dip[dir[l].inum];
							break;
						}
					}
				}
				if (flag == 1)
					break;
				if (j == sb->ninodes) {
					fprintf(stderr, "ERROR: inaccessible directory exists.\n");
					exit(1);
				}
			}
		}
	}

	// extra credit 1
	for (int i = 1; i < sb->ninodes + 1; i++) {
		if (i != 1 && dip[i].type == T_DIR) {
			for (int j = 0; j < NDIRECT; j++) {
				if (dip[i].addrs[j] == 0)
					continue;
				struct xv6_dirent *dir = (struct xv6_dirent *)(img_ptr + dip[i].addrs[j] * BSIZE);
				for (int k = 0; k < BSIZE / sizeof(struct xv6_dirent); k++) {
					if (!strcmp(dir[k].name, "..")) {
						struct dinode parent = dip[dir[k].inum];
						int contain = 0;
						for (int l = 0; l < NDIRECT; l++) {
							if (parent.addrs[l] == 0)
								continue;
							struct xv6_dirent *dir2 = (struct xv6_dirent *)(img_ptr + parent.addrs[l] * BSIZE);
							for (int m = 0; m < BSIZE / sizeof(struct xv6_dirent); m++) {

								if (dir2[m].inum == i)
									contain = 1;
							}
						}
						if (contain == 0) {
							fprintf(stderr, "ERROR: parent directory mismatch.\n");
							exit(1);
						}
					}
				}
			}
		}
	}
}

void repair() {
	char bm[BSIZE] = { 0 };
	int mapcount;
	for (mapcount = 0; mapcount < (sb->ninodes / IPB + 4) / 8; mapcount++) {
		bm[mapcount] = -1;
	}
	for (int i = 0; i < (sb->ninodes / IPB + 4) % 8; i++) {
		bm[mapcount] += 1 << i;
	}

	for (int i = 1; i < sb->ninodes + 1; i++) {
		if (i == 1) {
			if (dip[i].type != T_DIR || check_dir(&dip[i], 0)) {
//				fprintf(stderr, "ERROR: root directory does not exist.\n");
//				exit(1);
			}
		}
		if (dip[i].type == 0)
			continue;
		if (!(dip[i].type == T_FILE || dip[i].type == T_DIR || dip[i].type == T_DEV)) {
//			fprintf(stderr, "ERROR: bad inode.\n");
//			exit(1);
		}
		for (int j = 0; j < NDIRECT; j++)
		{
			if (dip[i].addrs[j] == 0)
				continue;
			if (dip[i].addrs[j] < (sb->ninodes / IPB + 4) || dip[i].addrs[j] > sb->size) {
//				fprintf(stderr, "ERROR: bad direct address in inode.\n");
//				exit(1);
			}
			if (((bitmap[(dip[i].addrs[j]) / 8] & (1 << (dip[i].addrs[j]) % 8))) == 0) {
//				fprintf(stderr, "ERROR: address used by inode but marked free in bitmap.\n");
//				exit(1);
			}
			if (((bm[dip[i].addrs[j] / 8] & (1 << (dip[i].addrs[j] % 8)))) == 0) {
				bm[dip[i].addrs[j] / 8] += 1 << (dip[i].addrs[j] % 8);
			}
			else {
//				fprintf(stderr, "ERROR: direct address used more than once.\n");
//				exit(1);
			}

		}

		if (dip[i].addrs[NDIRECT] != 0) {
			if (dip[i].addrs[NDIRECT] < (sb->ninodes / IPB + 4) || dip[i].addrs[NDIRECT] > sb->size) {
//				fprintf(stderr, "ERROR: bad indirect address in inode.\n");
//				exit(1);
			}
			if (((bm[dip[i].addrs[NDIRECT] / 8] & (1 << (dip[i].addrs[NDIRECT] % 8)))) == 0) {
//				bm[dip[i].addrs[NDIRECT] / 8] += 1 << (dip[i].addrs[NDIRECT] % 8);
			}
			else {
//				fprintf(stderr, "ERROR: indirect address used more than once.\n");
//				exit(1);
			}

			uint *indirect = (uint *)(img_ptr + dip[i].addrs[NDIRECT] * BSIZE);
			for (int j = 0; j < NINDIRECT; j++) {
				if (indirect[j] == 0)
					continue;
				if (indirect[j] < (sb->ninodes / IPB + 4) || indirect[j] > sb->size) {
//					fprintf(stderr, "ERROR: bad indirect address in inode.\n");
//					exit(1);
				}
				if (((bitmap[(indirect[j]) / 8] & (1 << (indirect[j]) % 8))) == 0) {
//					fprintf(stderr, "ERROR: address used by inode but marked free in bitmap.\n");
//					exit(1);
				}
				if (((bm[indirect[j] / 8] & (1 << (indirect[j] % 8)))) == 0) {
					bm[indirect[j] / 8] += 1 << (indirect[j] % 8);
				}
				else {
//					fprintf(stderr, "ERROR: indirect address used more than once.\n");
//					exit(1);
				}
			}
		}
		if (dip[i].type == T_DIR) {
			if (check_dir(&dip[i], 1)) {
//				fprintf(stderr, "ERROR: directory not properly formatted.\n");
//				exit(1);
			}
			if (check_dir(&dip[i], 2)) {
//				fprintf(stderr, "ERROR: inode referred to in directory but marked free.\n");
//				exit(1);
			}
			check_dir(&dip[i], 3);
		}
		inodes[inodes_count] = i;
		inodes_count++;
	}

	for (int i = 0; i < sb->size / 8; i++) {
		if (bm[i] != bitmap[i]) {
//			fprintf(stderr, "ERROR: bitmap marks block in use but it is not in use.\n");
//			exit(1);
		}
	}
	for (int i = 0; i < sb->ninodes; i++) {
		if (dip[i].type == 0)
			continue;
		if (dip[i].type == T_FILE && dip[i].nlink != reference_count[i]) {
//			fprintf(stderr, "ERROR: bad reference count for file.\n");
//			exit(1);
		}
	}
}

void lf(int node) {
	for (int i = 0; i < NDIRECT; i++) {
		if (dip[1].addrs[i] == 0)
			continue;
		struct xv6_dirent *dir = (struct xv6_dirent *)(img_ptr + dip[1].addrs[i] * BSIZE);
		for (int j = 0; j < BSIZE / sizeof(struct xv6_dirent); j++) {
			if (!strcmp(dir[j].name, "lost_found")) {
				struct xv6_dirent *lf_dir = (struct xv6_dirent *)(img_ptr + dip[dir[j].inum].addrs[0] * BSIZE);
				for (int k = 0; k < BSIZE / sizeof(struct xv6_dirent); k++) {
					if (lf_dir[k].inum == 0) {
						lf_dir[k].inum = (ushort)node;
						sprintf(lf_dir[k].name, "%d", node);
						if (dip[node].type == T_DIR) {
							for (int l = 0; l < NDIRECT; l++) {
								if (dip[node].addrs[l] == 0)
									continue;
								struct xv6_dirent *fix_parent = (struct xv6_dirent *)(img_ptr + dip[node].addrs[l] * BSIZE);
								for (int m = 0; m < BSIZE / sizeof(struct xv6_dirent); m++) {
									if (!strcmp(fix_parent[m].name, ".."))
										fix_parent[m].inum = dir[j].inum;
								}
							}
						}
						return;
					}
				}
			}
		}
	}
}

int main(int argc, char *argv[]) {
	int fd;
	if (argc == 2) {
		fd = open(argv[1], O_RDONLY);
	}
	else if (argc == 3){
		fd = open(argv[2], O_RDWR);
	}
	else {
		fprintf(stderr, "Usage: xcheck <file_system_image>\n");
		exit(1);
	}
	if (fd < 0) {
		fprintf(stderr, "image not found.\n");
		exit(1);
	}
	struct stat sbuf;
	fstat(fd, &sbuf);
	if (argc == 2) {
		img_ptr = mmap(NULL, sbuf.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	}
	else {
		img_ptr = mmap(NULL, sbuf.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	}
	if (img_ptr == (void *)-1) {
		fprintf(stderr, "image not found.\n");
		exit(1);
	}
	sb = (struct superblock *)(img_ptr + BSIZE);
	dip = img_ptr + 2 * BSIZE;
	bitmap = img_ptr + (sb->ninodes / IPB + 3)*BSIZE;
	inodes = (int *)calloc(sb->ninodes, sizeof(int));
	found = (int *)calloc(sb->ninodes, sizeof(int));
	reference_count = (int *)calloc(sb->ninodes, sizeof(int));
	inodes_count = 0;
	found_count = 0;
	if (argc == 2) {
		check();
		if (inodes_count > found_count) {
			fprintf(stderr, "ERROR: inode marked use but not found in a directory.\n");
			exit(1);
		}
		extra_credit();
	}
	if (argc == 3 && !strcmp(argv[1], "-r")) {
		repair();
		if (inodes_count > found_count) {
			for (int i = 0; i < inodes_count; i++) {
				for (int j = 0; j < found_count; j++) {
					if (inodes[i] == found[j])
						break;
					if (j == found_count - 1) {
						lf(inodes[i]);
					}
				}
			}
		}
	}
	//printf("%d %d\n", found_count, inodes_count);
	close(fd);
	free(inodes);
	free(found);
	free(reference_count);
	return 0;
}


