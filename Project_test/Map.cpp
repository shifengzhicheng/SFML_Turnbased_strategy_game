#include "Map.h"
/* ���ĳ������Χ�ж����ϰ���(ǽ) , cycleΪ����Ȧ��*/
int mapgenerator::checkNeighborWalls
(std::vector<std::vector <int> >& M, int lines, int cols, int line, int col, int cycle = 1)
{
    int count = 0;
    for (int i = line - cycle; i <= line + cycle; ++i)
        for (int j = col - cycle; j <= col + cycle; ++j)
        {
            if ((i >= 0 && i < lines) && (j >= 0 && j < cols))
                if (M[i][j] != M[line][col])
                    count++;
        }

    /* ��ǰ��Ϊǽ�򲻼��� */
    if (M[line][col] != 0)
        count--;

    return count;
}

void mapgenerator::gmap
(std::vector<std::vector <int> >& initMap, int cols, int lines) {

    /* ��ʼ����ͼ */
    std::default_random_engine e(time(0));
    std::uniform_real_distribution<double> u(0.0, 1.0);
    for (int line = 0; line < lines; ++line)
    {
        for (int col = 0; col < cols; ++col)
        {
            if (line == 0 || col == 0 || line == lines - 1 || col == cols - 1)
                initMap[line][col] = 1;
            else
                initMap[line][col] = u(e) < 0.40 ? 1 : 0;
        }
    }

    ///* ��ӡ��ͼ */
    //for (auto line : initMap) {
    //    for (auto col : line)
    //    {
    //        if (col == 0)
    //            std:://cout << ' ';
    //        else
    //            std:://cout << '*';
    //    }

    //    std:://cout << std::endl;
    //}
    //std:://cout << std::endl;

    /* ��ͼ�ݻ� */
    for (int loop = 0; loop < 5; ++loop)
    {
        /* ���ɵ�ͼ */
        for (int line = 0; line < lines; ++line)
        {
            for (int col = 0; col < cols; ++col)
            {
                /*
                1.�����ǰԪ����ǽ����ô��Χ���� 4 ��ǽ�ͱ���Ϊǽ
                2.�����ǰԪ���ǵذ壬��ô��Χ��һȦ���� 5 ��ǽ�ͱ�Ϊǽ������Χ�ڶ�ȦС��2���ͱ�Ϊǽ
                */
                int wallCount1 = checkNeighborWalls(initMap, lines, cols, line, col, 1);
                int wallCount2 = checkNeighborWalls(initMap, lines, cols, line, col, 2);

                if (initMap[line][col] == 1)
                {
                    initMap[line][col] = wallCount1 >= 4 ? 1 : 0;
                }
                else
                {
                    if (loop < 2)
                        initMap[line][col] = (wallCount1 >= 5) ? 1 : 0;
                    //loop<2,��һȦǽ�ܶ࣬��ô����ط���Ҫ������
                    else
                        initMap[line][col] = (wallCount1 >= 5 || wallCount2 <= 2) ? 1 : 0;
                    //��һȦǽ�ܶ࣬������ڶ�Ȧǽ���٣�����ط�Ӧ�ñ����ϣ���������ط�Ӧ�ñ�������
                }
                /* ����Χǽ�ڲ��� */
                if (line == 0 || col == 0 || line == lines - 1 || col == cols - 1)
                    initMap[line][col] = 1;
            }
        }
        for (int loop = 0; loop < 1; loop++) {
            for (int line = 0; line < lines; ++line)
            {
                for (int col = 0; col < cols; ++col)
                {
                    int wallCount1 = checkNeighborWalls(initMap, lines, cols, line, col, 1);
                    int wallCount2 = checkNeighborWalls(initMap, lines, cols, line, col, 2);
                    if (initMap[line][col] == 1)
                    {
                        //�����ɽ
                        initMap[line][col] = wallCount1 >= 3 ? 1 : rand() % 2 + 2;
                        //��һȦǽ�ܶ࣬��������Ǿͼ�����Ϊǽ����������������һ������
                    }
                    else if (initMap[line][col] == 2) {
                        //����Ǻ���
                        initMap[line][col] = wallCount1 >= 2 ? 2 : 3;
                        //��һȦ�����ܶ࣬��������Ǿͼ�����Ϊ��������������������һ����ľ
                    }
                    else if (initMap[line][col] == 3) {
                        //�������ľ
                        if (initMap[line][col] = wallCount1 <= 1) break;
                    }
                    else {
                        initMap[line][col] = (wallCount1 >= 5 || wallCount2 <= 2) ? 1 : 0;
                    }
                    if (line == 0 || col == 0 || line == lines - 1 || col == cols - 1)
                        initMap[line][col] = 1;
                }
            }
        }

        //    /* ��ӡ��ͼ */
        //    for (auto line : initMap) {
        //        for (auto col : line)
        //        {
        //            if (col == 0)
        //                std:://cout << ' ';
        //            else if (col == 1)
        //                std:://cout << '*';
        //            else if (col == 2)
        //                std:://cout << 'W';
        //            else if (col == 3)
        //                std:://cout << 'T';
        //        }
        //        std:://cout << std::endl;
        //    }
        //    std:://cout << std::endl;
        //}
    }
}