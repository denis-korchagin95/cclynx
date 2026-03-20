#ifndef CCLYNX_TARGET_ARM64_H
#define CCLYNX_TARGET_ARM64_H 1

struct ir_program;

void target_arm64_generate(struct ir_program * program, FILE * file);

#endif /* CCLYNX_TARGET_ARM64_H */
