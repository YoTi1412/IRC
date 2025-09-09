NAME    = ircserv
CC      = c++
CFLAGS  = -Wall -Werror -Wextra -std=c++98 -g -fsanitize=address

SRCS_PATH   = srcs/
CMD_PATH    = srcs/commands/
INC_PATH    = includes/

SRCS        = main.cpp \
              Server.cpp \
              Client.cpp \
              Channel.cpp \
              Message.cpp \
              Utils.cpp \
              Logger.cpp \
              commands/Invite.cpp \
              commands/Join.cpp \
              commands/Kick.cpp \
              commands/Mode.cpp \
              commands/Nick.cpp \
              commands/Pass.cpp \
              commands/Privmsg.cpp \
              commands/Topic.cpp \
              commands/User.cpp

OBJ_PATH    = objs/
OBJS        = $(addprefix $(OBJ_PATH), $(SRCS:.cpp=.o))

INCLUDES    = -I $(INC_PATH)

all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $(OBJS)

$(OBJ_PATH)%.o: $(SRCS_PATH)%.cpp
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -rf $(OBJ_PATH)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
