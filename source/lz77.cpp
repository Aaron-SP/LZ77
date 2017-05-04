/* Copyright (c) <2012-2015> <Aaron Springstroh, LZ77 Compression Tool>

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
claim that you wrote the original software. If you use this software
in a product, an acknowledgement in the product documentation would be
appreciated but is not required.

2. Altered source versions must be plainly marked as such, and must not be
misrepresented as being the original software.

3. This notice may not be removed or altered from any source distribution.

This file is part of LZ77 Compression Tool.

LZ77 Compression Tool is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

LZ77 Compression Tool is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with LZ77 Compression Tool.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <cmath>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <stdexcept>

const int WINDOW_SIZE = 4096;
const int DECOMP_BUFF_SIZE = 72;

class Llist
{
  public:
    Llist();
    int pos;
    Llist *next;
};

Llist::Llist()
{
    pos = -1;
    next = 0;
}

class HashTable
{
  public:
    HashTable();
    ~HashTable();
    void AddNode(const char *buff, int p);
    void RemoveNode(const char *buff, int p);
    Llist *SearchTable(const char *buff, int p);

  private:
    Llist *Table[256][256];
};

HashTable::HashTable()
{
    // Allocate hash table for storing character indices
    for (int i = 0; i < 256; i++)
    {
        for (int j = 0; j < 256; j++)
        {
            Table[i][j] = new Llist;
        }
    }
}

HashTable::~HashTable()
{
    // Delete all nodes in the hash table
    for (int i = 0; i < 256; i++)
    {
        for (int j = 0; j < 256; j++)
        {
            Llist *temp = Table[i][j];
            Llist *old = temp;
            while (temp->next != 0)
            {
                temp = temp->next;
                delete old;
                old = temp;
            }

            delete temp;
        }
    }
}

void HashTable::AddNode(const char *buff, int p)
{
    // Traverse the linked list to the end
    Llist *temp = Table[(unsigned)buff[p]][(unsigned)buff[p + 1]];
    while (temp->next != 0)
    {
        temp = temp->next;
    }

    // Add a node at the end of the linked list
    temp->pos = p;
    temp->next = new Llist;
}
void HashTable::RemoveNode(const char *buff, int p)
{
    // Remove a node in the hash table
    Llist *temp = Table[(unsigned)buff[p]][(unsigned)buff[p + 1]];
    Llist *old = temp;
    while (temp->pos != (int)p)
    {
        old = temp;
        temp = temp->next;
    }
    while (old == temp)
    {
        temp = temp->next;
        old->pos = temp->pos;
        old->next = temp->next;
    }

    delete temp;
}
Llist *HashTable::SearchTable(const char *buff, int p)
{
    // Look up match in table
    Llist *find = Table[(unsigned)buff[p]][(unsigned)buff[p + 1]];
    return find;
}

// Forward declaraction of main functions
void Compress(const char *buff, const int size, const std::string &fn);
int find_match(HashTable *hash, const char *buff, int &winstart, int &current, const int size);
void Decompress(const char *memblock, const int size, const std::string &filename);

class BitBuffer
{
  public:
    BitBuffer(const std::string &filename);
    ~BitBuffer();
    void addOne();
    void addZero();
    void addByte(char byte);
    void addBytes(int location, int len);
    void writeFooter();

  private:
    char bitbuff[8];
    int len;
    std::ofstream fout;
    void flushBuff();
};

BitBuffer::BitBuffer(const std::string &filename)
{
    len = 0;
    fout.open(filename, std::ios::out | std::ios::binary);
}

BitBuffer::~BitBuffer()
{
    flushBuff();
    writeFooter();
    flushBuff();
    fout.close();
}
void BitBuffer::addOne()
{
    bitbuff[len] = 49;
    len++;
    if (len == 8)
    {
        flushBuff();
        len = 0;
    }
}
void BitBuffer::addZero()
{
    bitbuff[len] = 48;
    len++;
    if (len == 8)
    {
        flushBuff();
        len = 0;
    }
}
void BitBuffer::addByte(char byte)
{
    for (int i = 0; i < 8; i++)
    {
        if (byte & 128)
        {
            addOne();
        }
        else
        {
            addZero();
        }

        byte <<= 1;
    }
}
void BitBuffer::addBytes(int location, int length)
{
    for (int i = 0; i < 12; i++)
    {
        if (location & 2048)
        {
            addOne();
        }
        else
        {
            addZero();
        }

        location <<= 1;
    }
    for (int i = 0; i < 5; i++)
    {
        if (length & 16)
        {
            addOne();
        }
        else
        {
            addZero();
        }

        length <<= 1;
    }
}
void BitBuffer::flushBuff()
{
    if (len == 8)
    {
        char temp = 0;
        for (int j = 0; j < 8; j++)
        {
            if (bitbuff[j] == 49)
            {
                temp = temp | 1;
            }
            else
            {
                temp = temp | 0;
            }
            if (j < 7)
            {
                temp <<= 1;
            }
        }

        fout.write(&temp, 1);
    }
}

void BitBuffer::writeFooter()
{
    char temp = 0;
    int length = len;
    for (int i = 0; i < length; i++)
    {
        if (bitbuff[i] == 49)
        {
            temp = temp | 1;
        }
        else
        {
            temp = temp | 0;
        }

        temp <<= 1;
    }

    len = 0;

    for (int i = 0; i < 8 - length; i++)
    {
        temp = temp | 0;
        if (i < 8 - length - 1)
        {
            temp <<= 1;
        }
    }
    addByte(temp);
    addByte(8 - length + 1);
}

int main(int argc, char *argv[])
{
    try
    {
        std::string filename;
        std::string mode;

        // Check if the input parameters are correct
        if (argc == 3)
        {
            mode = argv[1];
            filename = argv[2];
        }
        else
        {
            std::cout << "ERROR: Expected command format: lz77 <decompress|compress> <file_name>" << std::endl;
            return 1;
        }

        int size;
        std::unique_ptr<char> data;
        std::ifstream fin(filename, std::ios::in | std::ios::binary | std::ios::ate);
        if (fin.is_open())
        {
            size = fin.tellg();
            data = std::unique_ptr<char>(new char[size]);
            fin.seekg(0, std::ios::beg);
            fin.read(data.get(), size);
            fin.close();
        }
        else
        {
            throw std::runtime_error("Could not open input file '" + filename + "'");
        }

        if (mode.compare("compress") == 0)
        {
            // Send message to user of the mode we chose
            std::cout << "Compressing '" + filename + "'" << std::endl;
            Compress(data.get(), size, filename);
        }
        else if (mode.compare("decompress") == 0)
        {
            // Send message to user of the mode we chose
            std::cout << "Decompressing '" + filename + "'" << std::endl;
            Decompress(data.get(), size, filename);
        }
        else
        {
            std::cout << "Mode '" + mode + "' is not valid, expected 'compress' or 'decompress'" << std::endl;
        }
    }
    catch (std::exception &ex)
    {
        std::cout << ex.what() << std::endl;
    }

    return 0;
}

void Compress(const char *memblock, const int size, const std::string &fn)
{
    const std::string filename = fn + ".comp";
    int winstart = 0;
    int current = 0;
    int temp;
    int check;
    BitBuffer bits(filename);
    HashTable hash;
    int i = 0;
    while (i < size)
    {
        temp = current;
        check = find_match(&hash, memblock, winstart, current, size);
        if (check >= 0)
        {
            bits.addOne();
            bits.addBytes(check, current - temp - 4);
            i += current - temp;
        }
        else
        {
            bits.addZero();
            bits.addByte(memblock[current - 1]);
            i++;
        }
    }
}

int find_match(HashTable *hash, const char *buff, int &winstart, int &current, const int size)
{
    int max = 0;
    int maxloc = 0;
    int run;
    if (current == 0)
    {
        hash->AddNode(buff, current);
        (current)++;

        return -1;
    }
    else
    {
        Llist *search = hash->SearchTable(buff, current);
        while (search->pos != -1)
        {
            int k = 2;
            run = search->pos;
            while (buff[current + k] == buff[search->pos + k] && current + k < size)
            {
                k++;
                if (k > max)
                {
                    max = k;
                    maxloc = run;
                    if (max == 35)
                    {
                        break;
                    }
                }
            }

            if (max == 35)
            {
                break;
            }

            search = search->next;
        }

        if (max > 3)
        {
            for (int i = 0; i < max; i++)
            {
                hash->AddNode(buff, current);
                (current)++;
            }

            if (current - winstart > WINDOW_SIZE)
            {
                while (winstart != current - WINDOW_SIZE)
                {
                    hash->RemoveNode(buff, winstart);
                    (winstart)++;
                }
            }

            return current - max - maxloc;
        }
        else
        {
            hash->AddNode(buff, current);
            (current)++;
            if (current - winstart > WINDOW_SIZE)
            {
                while (winstart != current - WINDOW_SIZE)
                {
                    hash->RemoveNode(buff, winstart);
                    (winstart)++;
                }
            }

            return -1;
        }
    }
}

void Decompress(const char *memblock, const int size, const std::string &filename)
{
    char buff[DECOMP_BUFF_SIZE];
    int len = 0;
    int beg = 0;
    int end = 0;
    char cache[WINDOW_SIZE];
    int cacheend = 0;

    // Open output file
    const std::string decomp_file = filename + ".decomp";
    std::ofstream fout(decomp_file, std::ios::out | std::ios::binary);
    for (int i = 0; i < size; i++)
    {
        char temp;
        temp = memblock[i];
        for (int j = 7; j >= 0; j--)
        {
            if (temp & 1)
            {
                buff[end + j] = 49;
            }
            else
            {
                buff[end + j] = 48;
            }

            temp >>= 1;
        }

        end += 8;

        if (end == DECOMP_BUFF_SIZE)
        {
            end = 0;
        }

        len += 8;

        if (len > DECOMP_BUFF_SIZE)
        {
            std::cout << "Length overwrote data" << std::endl;
        }

        if (len + 8 >= DECOMP_BUFF_SIZE || i == size - 1)
        {
            while (len > 16)
            {
                if (buff[beg] == 48 && len >= 9)
                {
                    beg++;

                    if (beg == DECOMP_BUFF_SIZE)
                    {
                        beg = 0;
                    }

                    char byte = 0;

                    for (int j = 0; j < 8; j++)
                    {
                        if (buff[beg] == 48)
                        {
                            byte = byte | 0;
                        }
                        else
                        {
                            byte = byte | 1;
                        }

                        if (j < 7)
                        {
                            byte <<= 1;
                        }

                        beg++;

                        if (beg == DECOMP_BUFF_SIZE)
                        {
                            beg = 0;
                        }
                    }

                    if (fout.is_open())
                    {
                        fout.write(&byte, 1);
                        cache[cacheend] = byte;
                        cacheend++;
                        if (cacheend == WINDOW_SIZE)
                        {
                            cacheend = 0;
                        }
                    }

                    len -= 9;
                }
                else if (buff[beg] == 49 && len >= 18)
                {
                    beg++;
                    if (beg == DECOMP_BUFF_SIZE)
                    {
                        beg = 0;
                    }

                    int location = 0;

                    for (int j = 0; j < 12; j++)
                    {
                        if (buff[beg] == 48)
                        {
                            location = location | 0;
                        }
                        else
                        {
                            location = location | 1;
                        }

                        if (j < 11)
                        {
                            location <<= 1;
                        }

                        beg++;
                        if (beg == DECOMP_BUFF_SIZE)
                        {
                            beg = 0;
                        }
                    }

                    int length = 0;

                    for (int j = 0; j < 5; j++)
                    {
                        if (buff[beg] == 48)
                        {
                            length = length | 0;
                        }
                        else
                        {
                            length = length | 1;
                        }

                        if (j < 4)
                        {
                            length <<= 1;
                        }

                        beg++;

                        if (beg == DECOMP_BUFF_SIZE)
                        {
                            beg = 0;
                        }
                    }

                    length += 4;

                    len -= 18;

                    char byte = 0;

                    for (int j = 0; j < length; j++)
                    {
                        int tem = cacheend - location;
                        if (tem < 0)
                        {
                            tem += WINDOW_SIZE;
                        }

                        byte = cache[tem];

                        if (fout.is_open())
                        {
                            fout.write(&byte, 1);
                            cache[cacheend] = byte;
                            cacheend++;
                            if (cacheend == WINDOW_SIZE)
                            {
                                cacheend = 0;
                            }
                        }
                    }
                }
                else
                {
                    break;
                }
            }
        }
    }
}
