TARGET  = kbd-wdog
CFLAGS  = -Wall -Wextra -Os

$(TARGET): watchdog.c
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(TARGET)

.PHONY: clean
