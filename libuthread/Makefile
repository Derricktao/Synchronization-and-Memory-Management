targets	:= libuthread.a
objs	:= queue.o sem.o tps.o

CC	:= gcc
CFLAGS	:= -Wall -Werror
CFLAGS	+= -g
STATIC_BUILD := ar rcs

ifneq ($(V),1)
	Q = @
endif

all: $(targets)

deps	:= $(patsubst %.o,%.d,$(objs))
-include $(deps)
DEPFLAGS = -MMD -MF $(@:.o=.d)

libuthread.a: $(objs)
	@echo "STATIC_BUILD $@"
	$(Q)$(STATIC_BUILD) $(@) $(^)
%.o: %.c
	@echo "CC $@"
	$(Q)$(CC) $(CFLAGS) -o $@ -c $< $(DEPFLAGS)
clean:
	@echo "clean"
	$(Q)rm -f $(targets) $(objs) $(deps)
