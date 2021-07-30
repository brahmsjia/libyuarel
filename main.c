#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "yuarel.h"

int main(int argc, char * argv[])
{
	int ny;
	TS_YL yl;

	TS_YP yp[256] = {'\0'};

    char url[256] = "wlanuserip=userip&wlanusermac=usermac&uservlan&wlanacname&wlanacip&apmac&ssid&sn&hid&url";
    char key[256] = "foo";
    if(argc >= 2)
        sprintf(url, "%s", argv[1]);
    if(argc >= 3) {
        sprintf(key, "%s", argv[2]);
    }

    printf("url: %s\n", url);
    printf("key: %s\n", key);

	ny = yuarel_parse_query(url, '&', yp, 256);
    printf("ny: %d\n", ny);
    int i = 0;
	while (i < ny && yp[i].key!=NULL) {
        printf("%s: %s\n", yp[i].key, yp[i].val == NULL ? "": yp[i].val);
        i++;
    }
    
    char json[256] = {'\0'};
    build_json(yp, json);
    printf("%s\n", json);
    build_query(yp, json);
    printf("%s\n", json);

    char s[] = "http://baidu.com?a=b&c=d&k=中国";
    char d[256] = {'\0'};
    printf("s: %s\n", s);
    php_url_encode(s, d);
    printf("encode: %s\n", d);
    php_url_decode(d);
    printf("decode: %s\n", d);
}

