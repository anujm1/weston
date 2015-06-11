/*
 * Copyright © 2013 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "config.h"

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <unistd.h>

#include "config-parser.h"

static struct weston_config *
run_test(const char *text)
{
	struct weston_config *config;
	char file[] = "/tmp/weston-config-parser-test-XXXXXX";
	int fd, len;

	fd = mkstemp(file);
	len = write(fd, text, strlen(text));
	assert(len == (int) strlen(text));

	config = weston_config_parse(file);
	close(fd);
	unlink(file);

	return config;
}

static const char t0[] =
	"# nothing in this file...\n";

static const char t1[] =
	"# comment line here...\n"
	"\n"
	"[foo]\n"
	"a=b\n"
	"name=  Roy Batty    \n"
	"\n"
	"\n"
	"[bar]\n"
	"# more comments\n"
	"number=5252\n"
	"flag=false\n"
	"\n"
	"[stuff]\n"
	"flag=     true \n"
	"\n"
	"[bucket]\n"
	"color=blue \n"
	"contents=live crabs\n"
	"pinchy=true\n"
	"\n"
	"[bucket]\n"
	"material=plastic \n"
	"color=red\n"
	"contents=sand\n";

static const char *section_names[] = {
	"foo", "bar", "stuff", "bucket", "bucket"
};

static const char t2[] =
	"# invalid section...\n"
	"[this bracket isn't closed\n";

static const char t3[] =
	"# line without = ...\n"
	"[bambam]\n"
	"this line isn't any kind of valid\n";

static const char t4[] =
	"# starting with = ...\n"
	"[bambam]\n"
	"=not valid at all\n";

int main(int argc, char *argv[])
{
	struct weston_config *config;
	struct weston_config_section *section;
	const char *name;
	char *s;
	int r, b, i;
	int32_t n;
	uint32_t u;

	config = run_test(t0);
	assert(config);
	weston_config_destroy(config);

	config = run_test(t1);
	assert(config);
	section = weston_config_get_section(config, "mollusc", NULL, NULL);
	assert(section == NULL);

	section = weston_config_get_section(config, "foo", NULL, NULL);
	r = weston_config_section_get_string(section, "a", &s, NULL);
	assert(r == 0 && strcmp(s, "b") == 0);
	free(s);

	section = weston_config_get_section(config, "foo", NULL, NULL);
	r = weston_config_section_get_string(section, "b", &s, NULL);
	assert(r == -1 && errno == ENOENT && s == NULL);

	section = weston_config_get_section(config, "foo", NULL, NULL);
	r = weston_config_section_get_string(section, "name", &s, NULL);
	assert(r == 0 && strcmp(s, "Roy Batty") == 0);
	free(s);

	section = weston_config_get_section(config, "bar", NULL, NULL);
	r = weston_config_section_get_string(section, "a", &s, "boo");
	assert(r == -1 && errno == ENOENT && strcmp(s, "boo") == 0);
	free(s);

	section = weston_config_get_section(config, "bar", NULL, NULL);
	r = weston_config_section_get_int(section, "number", &n, 600);
	assert(r == 0 && n == 5252);

	section = weston_config_get_section(config, "bar", NULL, NULL);
	r = weston_config_section_get_int(section, "+++", &n, 700);
	assert(r == -1 && errno == ENOENT && n == 700);

	section = weston_config_get_section(config, "bar", NULL, NULL);
	r = weston_config_section_get_uint(section, "number", &u, 600);
	assert(r == 0 && u == 5252);

	section = weston_config_get_section(config, "bar", NULL, NULL);
	r = weston_config_section_get_uint(section, "+++", &u, 600);
	assert(r == -1 && errno == ENOENT && u == 600);

	section = weston_config_get_section(config, "bar", NULL, NULL);
	r = weston_config_section_get_bool(section, "flag", &b, 600);
	assert(r == 0 && b == 0);

	section = weston_config_get_section(config, "stuff", NULL, NULL);
	r = weston_config_section_get_bool(section, "flag", &b, -1);
	assert(r == 0 && b == 1);

	section = weston_config_get_section(config, "stuff", NULL, NULL);
	r = weston_config_section_get_bool(section, "bonk", &b, -1);
	assert(r == -1 && errno == ENOENT && b == -1);

	section = weston_config_get_section(config, "bucket", "color", "blue");
	r = weston_config_section_get_string(section, "contents", &s, NULL);
	assert(r == 0 && strcmp(s, "live crabs") == 0);
	free(s);

	section = weston_config_get_section(config, "bucket", "color", "red");
	r = weston_config_section_get_string(section, "contents", &s, NULL);
	assert(r == 0 && strcmp(s, "sand") == 0);
	free(s);

	section = weston_config_get_section(config, "bucket", "color", "pink");
	assert(section == NULL);
	r = weston_config_section_get_string(section, "contents", &s, "eels");
	assert(r == -1 && errno == ENOENT && strcmp(s, "eels") == 0);
	free(s);

	section = NULL;
	i = 0;
	while (weston_config_next_section(config, &section, &name))
		assert(strcmp(section_names[i++], name) == 0);
	assert(i == 5);

	weston_config_destroy(config);

	config = run_test(t2);
	assert(config == NULL);

	config = run_test(t3);
	assert(config == NULL);

	config = run_test(t4);
	assert(config == NULL);

	weston_config_destroy(NULL);
	assert(weston_config_next_section(NULL, NULL, NULL) == 0);

	section = weston_config_get_section(NULL, "bucket", NULL, NULL);
	assert(section == NULL);

	return 0;
}
