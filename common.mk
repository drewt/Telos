.PHONY: clean depclean maintainer-clean

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(AR) rcs $@ $^

clean:
	rm -f $(OBJECTS) $(TARGET)

depclean:
	rm -f $(OBJECTS:.o=.d)

maintainer-clean: clean depclean

-include $(OBJECTS:.o=.d)
