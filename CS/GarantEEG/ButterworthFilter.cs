using System;
using System.IO;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Net;
using System.Net.Sockets;
using TinyJson;
using System.Diagnostics;
using System.Threading;

namespace GarantEEG
{
    public class CButterworthFilter : IFilter
    {
        //! Фильтр
        private BandpassFilter m_Filter = null;

        //! Рабочая частота
        protected int m_Rate = 500;

        //! Частота среза
        protected int m_Frequency = 40;

        /**
		 * @brief CButterworthFilter Конструктор
		 * @param rate Рабочая частота
		 * @param frequency Нижняя частота среза
		 */
        public CButterworthFilter(int rate, int frequency)
        {
            m_Rate = rate;
            m_Frequency = frequency;
            m_Filter = new BandpassFilter(m_Rate, (double)m_Frequency);
        }

        ~CButterworthFilter()
        {
        }

        /**
         * @brief Type Получить тип фильтра
         * @return Тип фильтра
         */
        public int Type()
        {
            return (int)GARANT_EEG_FILTER_TYPE.FT_BUTTERWORTH;
        }

        /**
         * @brief Rate Получить рабочую частоту фильтра
         * @return Частота
         */
        public int Rate()
        {
            return m_Rate;
        }

        /**
         * @brief Frequency Получить частоту среза
         * @return Частота
         */
        public int Frequency()
        {
            return m_Frequency;
        }

        /**
		 * @brief Process Функция фильтрации данных
		 * @param input Данные
		 */
        public void Process(float[] input)
        {
            if (m_Filter != null)
                m_Filter.Process(input);
        }

        //--------------------------------------------------------------------------
        // This function returns the data filtered. Converted to C# 2 July 2014.
        // Original source written in VBA for Microsoft Excel, 2000 by Sam Van
        // Wassenbergh (University of Antwerp), 6 june 2007.
        //--------------------------------------------------------------------------
        public static double[] Butterworth(double[] indata, double deltaTimeinsec, double CutOff)
        {
            if (indata == null) return null;
            if (CutOff == 0) return indata;

            double Samplingrate = 1 / deltaTimeinsec;
            long dF2 = indata.Length - 1;        // The data range is set with dF2
            double[] Dat2 = new double[dF2 + 4]; // Array with 4 extra points front and back
            double[] data = indata; // Ptr., changes passed data

            // Copy indata to Dat2
            for (long r = 0; r < dF2; r++)
            {
                Dat2[2 + r] = indata[r];
            }
            Dat2[1] = Dat2[0] = indata[0];
            Dat2[dF2 + 3] = Dat2[dF2 + 2] = indata[dF2];

            const double pi = 3.14159265358979;
            double wc = Math.Tan(CutOff * pi / Samplingrate);
            double k1 = 1.414213562 * wc; // Sqrt(2) * wc
            double k2 = wc * wc;
            double a = k2 / (1 + k1 + k2);
            double b = 2 * a;
            double c = a;
            double k3 = b / k2;
            double d = -2 * a + k3;
            double e = 1 - (2 * a) - k3;

            // RECURSIVE TRIGGERS - ENABLE filter is performed (first, last points constant)
            double[] DatYt = new double[dF2 + 4];
            DatYt[1] = DatYt[0] = indata[0];
            for (long s = 2; s < dF2 + 2; s++)
            {
                DatYt[s] = a * Dat2[s] + b * Dat2[s - 1] + c * Dat2[s - 2]
                           + d * DatYt[s - 1] + e * DatYt[s - 2];
            }
            DatYt[dF2 + 3] = DatYt[dF2 + 2] = DatYt[dF2 + 1];

            // FORWARD filter
            double[] DatZt = new double[dF2 + 2];
            DatZt[dF2] = DatYt[dF2 + 2];
            DatZt[dF2 + 1] = DatYt[dF2 + 3];
            for (long t = -dF2 + 1; t <= 0; t++)
            {
                DatZt[-t] = a * DatYt[-t + 2] + b * DatYt[-t + 3] + c * DatYt[-t + 4]
                            + d * DatZt[-t + 1] + e * DatZt[-t + 2];
            }

            // Calculated points copied for return
            for (long p = 0; p < dF2; p++)
            {
                data[p] = DatZt[p];
            }

            return data;
        }
    }
}
