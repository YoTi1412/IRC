NAME        = ircserv
CC          = c++
CFLAGS      = -Wall -Werror -Wextra -std=c++98 -g -fsanitize=address

HEADERS     = $(addprefix $(INC_PATH), Channel.hpp Client.hpp Command.hpp Includes.hpp Logger.hpp Message.hpp Replies.hpp Server.hpp Utils.hpp)
BONUS_HEADERS = $(addprefix $(BONUS_PATH), Bot.hpp PlayerStats.hpp Room.hpp)

SRCS_PATH   = srcs/
CMD_PATH    = srcs/commands/
INC_PATH    = includes/

SRCS        = main.cpp \
              Server.cpp \
              Client.cpp \
              Channel.cpp \
              Utils.cpp \
              Logger.cpp \
              commands/CommandUtils.cpp \
              commands/Invite.cpp \
              commands/Join.cpp \
              commands/Kick.cpp \
              commands/Mode.cpp \
              commands/Nick.cpp \
              commands/Part.cpp \
              commands/Pass.cpp \
              commands/Privmsg.cpp \
              commands/Topic.cpp \
              commands/Name.cpp \
              commands/Ping.cpp \
              commands/Quit.cpp \
              commands/User.cpp

OBJ_PATH    = objs/
OBJS        = $(addprefix $(OBJ_PATH), $(SRCS:.cpp=.o))

BONUS_PATH      = bot/
BONUS_OBJ_PATH  = bot/objs/
BONUS_SRCS      = srcs/main.cpp \
                  srcs/Bot.cpp \
                  srcs/PlayerStats.cpp \
                  srcs/Room.cpp \
                  srcs/handleGameCore.cpp \
                  srcs/handleMultiplayer.cpp \
                  srcs/utils.cpp
BONUS_OBJS      = $(addprefix $(BONUS_OBJ_PATH), $(BONUS_SRCS:.cpp=.o))

INCLUDES    = -I $(INC_PATH)

all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $(OBJS)

$(OBJ_PATH)%.o: $(SRCS_PATH)%.cpp $(HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BONUS_OBJ_PATH)%.o: $(BONUS_PATH)%.cpp $(BONUS_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

bonus: $(NAME) bot/cisor_bot

bot/cisor_bot: $(BONUS_OBJS)
	$(CC) $(CFLAGS) -o $@ $(BONUS_OBJS)

clean:
	rm -rf $(OBJ_PATH)
	rm -rf $(BONUS_OBJ_PATH)

fclean: clean
	rm -f $(NAME) bot/cisor_bot

re: fclean all

.PHONY: all clean fclean re bonus
