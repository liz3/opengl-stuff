#ifndef CURSOR_H
#define CURSOR_H

#include <string>
#include <map>
#include <vector>
#include <sstream>
#include <fstream>
#include "font_atlas.h"
#include <deque>
struct PosEntry {
  int x,y, skip;
};
struct HistoryEntry {
  int x,y;
  int mode;
  int length;
  std::string content;
  std::vector<std::string> extra;
};
class Cursor {
 public:
  std::vector<std::string> lines;
  std::map<std::string, PosEntry> saveLocs;
  std::deque<HistoryEntry> history;
  int x = 0;
  int y = 0;
  int xSave = 0;
  int skip = 0;
  float xSkip = 0;
  float height = 0;
  float lineHeight = 0;
  int maxLines = 0;
  std::string* bind = nullptr;
  void setBounds(float height, float lineHeight) {
    this->height = height;
    this->lineHeight = lineHeight;
    maxLines = std::floor(height / lineHeight) -1;
  }
  void bindTo(std::string* entry) {
    bind = entry;
    xSave = x;
    x = entry->length();
  }
  void unbind() {
    bind = nullptr;
    x = xSave;
  }
  std::string search(std::string what, bool skipFirst, bool shouldOffset = true) {
    int i = shouldOffset ? y : 0;
    bool found = false;
    for(int x = i; x < lines.size(); x++) {
      auto line = lines[x];
      auto where = line.find(what);
      if(where != std::string::npos) {
        if(skipFirst && !found) {
          found = true;
          continue;
        }
        y = x;
        // we are in non 0 mode here, set savex
        xSave = where;
        center(i);
        return "[At: " + std::to_string(y + 1) + ":" + std::to_string(where + 1) + "]: ";
      }
      i++;
    }
    if(skipFirst)
      return "[No further matches]: ";
    return "[Not found]: ";
  }
  int findAnyOf(std::string str, std::string what) {
    std::string::const_iterator c;
    int offset = 0;
    for (c = str.begin(); c != str.end(); c++) {

      if(c != str.begin() && what.find(*c) != std::string::npos) {
        return offset;
      }
      offset++;
    }

    return -1;
  }
  int findAnyOfLast(std::string str, std::string what) {
    std::string::const_iterator c;
    int offset = 0;
    for (c = str.end()-1; c != str.begin(); c--) {

      if(c != str.end()-1 && what.find(*c) != std::string::npos) {
        return offset;
      }
      offset++;
    }

    return -1;
  }

  void advanceWord() {
    std::string* target = bind ? bind : &lines[y];
    int offset = findAnyOf(target->substr(x), " \t\n[]{}/\\*()=_-,.");
    if(offset == -1)
      x = target->length();
    else
      x+= offset;

  }
  std::string deleteWord() {
    std::string* target = bind ? bind : &lines[y];
    int offset = findAnyOf(target->substr(x), " \t\n[]{}/\\*()=_-.,");
    if(offset == -1)
      offset = target->length() -x;
    offset++;
    std::string w = target->substr(x,offset);
    target->erase(x, offset);
    historyPush(3, w.length(), w);
    return w;
  }
  bool undo() {
    if(history.size() == 0)
      return false;
    HistoryEntry entry = history[0];
    history.pop_front();
    // //lets still bounds check
    // if(lines.size() < entry.y || lines[entry.y].length() < entry.x)
    //   return false;
    switch(entry.mode) {

      case 3: {
        x = entry.x;
        y = entry.y;
        center(y);
        (&lines[y])->insert(x, entry.content);
        x += entry.length;
        break;
      }
      case 4: {
        y = entry.y;
        x = entry.x;
        center(y);
        (&lines[y])->insert(x-1, entry.content);
        break;
      }
      case 5: {
        y = entry.y;
        x = 0;
        lines.insert(lines.begin()+y, entry.content);
        center(y);
        if(entry.extra.size())
          lines[y-1] = entry.extra[0];
        break;
      }
      case 6: {
        y = entry.y;
        x = (&lines[y])->length();
        lines.erase(lines.begin()+y+1);
        center(y);
        break;
      }
      case 7: {
        y = entry.y;
        x = 0;
        if(entry.extra.size()) {
          lines[y] = entry.content + entry.extra[0];
          lines.erase(lines.begin()+y+1);
        } else {
        lines.erase(lines.begin()+y);
        }
        center(y);
        break;
      }
      case 8: {
        y = entry.y;
        x = entry.x;
        (&lines[y])->erase(x, 1);
        center(y);
        break;
      }
      default:
        return false;
    }
    return true;
  }


  void advanceWordBackwards() {
    std::string* target = bind ? bind : &lines[y];
    int offset = findAnyOfLast(target->substr(0,x), " \t\n[]{}/\\*()=_-.,");
    if(offset == -1)
      x = 0;
    else
      x -= offset;

  }

  void gotoLine(int l) {
    if(l-1 > lines.size())
      return;
    x = 0;
    xSave = 0;
    y = l-1;
    center(l);
  }
  void center(int l) {
    if(l < maxLines /2 || lines.size() < l)
      skip = 0;
    else
      skip = l - (maxLines / 2);


  }
  std::vector<std::string> split(std::string base, std::string delimiter) {
    std::vector<std::string> final;
    size_t pos = 0;
    std::string token;
    while ((pos = base.find(delimiter)) != std::string::npos) {
      token = base.substr(0, pos);
      final.push_back(token);
      base.erase(0, pos + delimiter.length());
    }
    final.push_back(base);
    return final;
  }

  Cursor() {
    lines.push_back("");
  }
  // Cursor(std::vector<std::string> contents) {
  //   lines = contents;

   Cursor(std::string path) {
     std::stringstream ss;
     std::ifstream stream(path);
     ss << stream.rdbuf();
     lines = split(ss.str(), "\n");
     stream.close();

  }
  void historyPush(int mode, int length, std::string content) {
    if(bind != nullptr)
      return;
    HistoryEntry entry;
    entry.x = x;
    entry.y = y;
    entry.mode = mode;
    entry.length = length;
    entry.content = content;
    if(history.size() > 5000)
      history.pop_back();
    history.push_front(entry);
  }
  void historyPushWithExtra(int mode, int length, std::string content, std::vector<std::string> extra) {
    if(bind != nullptr)
      return;
    HistoryEntry entry;
    entry.x = x;
    entry.y = y;
    entry.mode = mode;
    entry.length = length;
    entry.content = content;
    entry.extra = extra;
    if(history.size() > 5000)
      history.pop_back();
    history.push_front(entry);
  }

  bool openFile(std::string oldPath, std::string path) {
    std::ifstream stream(path);
    if(!stream.is_open()) {
      return false;
    }
    if(oldPath.length()) {
      PosEntry entry;
      entry.x = xSave;
      entry.y = y;
      entry.skip = skip;
      saveLocs[oldPath] = entry;
    }
    if(saveLocs.count(path)) {
        PosEntry savedEntry = saveLocs[path];
        x = savedEntry.x;
        y = savedEntry.y;
        skip = savedEntry.skip;
    } else {
      {
        // this is purely for the buffer switcher
        PosEntry idk{0,0,0};
        saveLocs[path] = idk;
      }
      x=0;
      y=0;
      skip = 0;
   }
    xSave = x;
    history.clear();
    std::stringstream ss;
    ss << stream.rdbuf();
    lines = split(ss.str(), "\n");
    if(skip > lines.size() - maxLines)
      skip = 0;
    stream.close();
    return true;
  }
  void append(char c) {
    if(c == '\n' && bind == nullptr) {
      auto pos = lines.begin() + y;
      std::string* current = &lines[y];
      bool isEnd = x == current->length();
      if(isEnd) {
        lines.insert(pos+1,"");
        historyPush(6, 0, "");
      } else {
        if(x== 0) {
          lines.insert(pos, "");
          historyPush(7, 0, "");
        } else {
          std::string toWrite = current->substr(0, x);
          std::string next = current->substr(x);
          lines[y] = toWrite;
          lines.insert(pos+1, next);
          historyPushWithExtra(7, toWrite.length(), toWrite, {next});
        }

      }
      y++;
      x = 0;
    }else {
      auto* target = bind ? bind :  &lines[y];
      std::string content;
      content += c;
      target->insert(x, content);
      historyPush(8, 1, content);
      x++;
    }
  }
  void append(std::string content) {
    auto* target = bind ? bind : &lines[y];
    target->insert(x, content);
    x += content.length();
  }

  std::string getCurrentAdvance(bool useSaveValue = false) {
    if(useSaveValue)
      return lines[y].substr(0, xSave);

    if(bind)
      return bind->substr(0, x);
    return lines[y].substr(0, x);
  }
  void removeOne() {
    std::string* target = bind ? bind :  &lines[y];
    if(x == 0) {
      if(y == 0 || bind)
        return;

        std::string* copyTarget = &lines[y-1];
        int xTarget = copyTarget->length();
        if (target->length() > 0) {
          historyPushWithExtra(5, (&lines[y])->length(), lines[y], {lines[y-1]});
          copyTarget->append(*target);
        } else {
          historyPush(5, (&lines[y])->length(), lines[y]);
        }
        lines.erase (lines.begin()+y);

        y--;
        x = xTarget;
    } else {
      historyPush(4, 1, std::string(1, (*target)[x-1]));
      target->erase(x-1, 1);
      x--;
    }
  }
  void moveUp() {
    if(y == 0 || bind)
      return;
   std::string* target = &lines[y-1];
   int targetX = target->length() < x ? target->length() : x;
   x = targetX;
   y--;
  }
  void moveDown() {
    if(bind || y == lines.size()-1)
      return;
   std::string* target = &lines[y+1];
   int targetX = target->length() < x ? target->length() : x;
   x = targetX;
   y++;
  }

  void jumpStart() {
    x = 0;
  }

  void jumpEnd() {
    if(bind)
      x = bind->length();
    else
      x = lines[y].length();
  }


  void moveRight() {
    std::string* current = bind ? bind : &lines[y];
    if(x == current->length()) {
      if(y == lines.size()-1 || bind)
        return;
      y++;
      x = 0;
    } else {
      x++;
    }
  }
  void moveLeft() {
    std::string* current = bind ? bind :  &lines[y];
    if(x == 0) {
      if(y == 0 || bind)
        return;
      std::string* target = & lines[y-1];
      y--;
      x = target->length();
    } else {
      x--;
    }
  }
  bool saveTo(std::string path) {
    std::ofstream stream(path,  std::ofstream::out);
    if(!stream.is_open()) {
      return false;
    }
    for(size_t i = 0; i < lines.size(); i++) {
      stream << lines[i];
      if(i < lines.size() -1)
        stream << "\n";
    }
    stream.flush();
    stream.close();
    return true;
  }
  std::string getContent(FontAtlas* atlas, float maxWidth) {

    std::vector<std::string> prepare;
    int end = skip + maxLines;
    if(end >= lines.size()) {
      end = lines.size();
      skip = end-maxLines;
      if(skip < 0)
        skip = 0;
    }
    if(y == end && end < (lines.size())) {

      skip++;
      end++;

    } else if(y < skip && skip > 0) {
      skip--;
      end--;
    }
    for(size_t i = skip; i < end; i++) {
      prepare.push_back(lines[i]);
    }
    float neededAdvance = atlas->getAdvance(lines[y].substr(0,x));
    float totalAdvance = atlas->getAdvance(lines[y]);
    int xOffset = 0;
    if(neededAdvance > maxWidth) {
      auto all = atlas->getAllAdvance(lines[y]);
      float acc = 0;
      xSkip = 0;
      for(auto value : all) {
        if(acc > neededAdvance){
          xSkip += value;
          xOffset++;
          break;
        }
        // fk me
        if(acc > maxWidth) {
          xOffset++;
          xSkip += value;
        }
        acc += value;
      }
    } else {
      xSkip = 0;
    }

    std::stringstream ss;
    for(size_t i = 0; i < prepare.size(); i++) {
      if(xOffset > 0) {
        auto a = prepare[i];
        if(a.length() > xOffset)
          ss << a.substr(xOffset);
      } else {
        ss << prepare[i];
      }
      if(i < end -1)
        ss << "\n";
    }
    return ss.str();
  }
  std::vector<std::string> getSaveLocKeys() {
    std::vector<std::string> ls;
    for(std::map<std::string, PosEntry>::iterator it = saveLocs.begin(); it != saveLocs.end(); ++it) {
      ls.push_back(it->first);
}
    return ls;
  }
};
#endif