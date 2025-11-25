#include "include/wp_pool.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

int main() {
    init_pool();
    void *a = wpmalloc(100);
    void *b = wpmalloc(200);
    printf("a=%p b=%p\n", a, b);
    assert(a && b && a != b);
    strcpy((char *) a, "hello");
    wpfree(a);
    void *c = wpmalloc(80); // should reuse a's chunk (100 >= 80)
    printf("c=%p (should equal a) value='%s'\n", c, (char *) c);
    assert(c == a);
    wpfree(b);
    void *d = wpmalloc(150); // should reuse b's chunk (200 >= 150)
    printf("d=%p (should equal b)\n", d);
    assert(d == b);
    // allocate something bigger than existing free chunks sizes to force new chunk
    void *e = wpmalloc(50000);
    printf("e=%p (new allocation)\n", e);
    assert(e && e != a && e != b);
    destroy_pool();
    printf("Pool test passed.\n");
    return 0;
}
