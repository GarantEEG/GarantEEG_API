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
    /// 

    /// Represents a biquad-filter.
    /// 

    public abstract class BiQuad
    {
        /// 

        /// The a0 value.
        /// 

        protected double A0;
        /// 

        /// The a1 value.
        /// 

        protected double A1;
        /// 

        /// The a2 value.
        /// 

        protected double A2;
        /// 

        /// The b1 value.
        /// 

        protected double B1;
        /// 

        /// The b2 value.
        /// 

        protected double B2;
        /// 

        /// The q value.
        /// 

        private double _q;
        /// 

        /// The gain value in dB.
        /// 

        private double _gainDB;
        /// 

        /// The z1 value.
        /// 

        protected double Z1;
        /// 

        /// The z2 value.
        /// 

        protected double Z2;

        private double _frequency;

        /// 

        /// Gets or sets the frequency.
        /// 

        /// value;The samplerate has to be bigger than 2 * frequency.
        public double Frequency
        {
            get { return _frequency; }
            set
            {
                if (SampleRate < value * 2)
                {
                    throw new ArgumentOutOfRangeException("value", "The samplerate has to be bigger than 2 * frequency.");
                }
                _frequency = value;
                CalculateBiQuadCoefficients();
            }
        }

        /// 

        /// Gets the sample rate.
        /// 

        public int SampleRate { get; private set; }

        /// 

        /// The q value.
        /// 

        public double Q
        {
            get { return _q; }
            set
            {
                if (value <= 0)
                {
                    throw new ArgumentOutOfRangeException("value");
                }
                _q = value;
                CalculateBiQuadCoefficients();
            }
        }

        /// 

        /// Gets or sets the gain value in dB.
        /// 

        public double GainDB
        {
            get { return _gainDB; }
            set
            {
                _gainDB = value;
                CalculateBiQuadCoefficients();
            }
        }

        /// 

        /// Initializes a new instance of the  class.
        /// 

        /// The sample rate.
        /// The frequency.
        /// 
        /// sampleRate
        /// or
        /// frequency
        /// or
        /// q
        /// 
        protected BiQuad(int sampleRate, double frequency)
            : this(sampleRate, frequency, 1.0 / Math.Sqrt(2))
        {
        }

        /// 

        /// Initializes a new instance of the  class.
        /// 

        /// The sample rate.
        /// The frequency.
        /// The q.
        /// 
        /// sampleRate
        /// or
        /// frequency
        /// or
        /// q
        /// 
        protected BiQuad(int sampleRate, double frequency, double q)
        {
            if (sampleRate <= 0)
                throw new ArgumentOutOfRangeException("sampleRate");
            if (frequency <= 0)
                throw new ArgumentOutOfRangeException("frequency");
            if (q <= 0)
                throw new ArgumentOutOfRangeException("q");
            SampleRate = sampleRate;
            Frequency = frequency;
            Q = q;
            GainDB = 6;
        }

        /// 

        /// Processes a single  sample and returns the result.
        /// 

        /// The input sample to process.
        /// The result of the processed  sample.
        public float Process(float input)
        {
            double o = input * A0 + Z1;
            Z1 = input * A1 + Z2 - B1 * o;
            Z2 = input * A2 - B2 * o;
            return (float)o;
        }

        /// 

        /// Processes multiple  samples.
        /// 

        /// The input samples to process.
        /// The result of the calculation gets stored within the  array.
        public void Process(float[] input)
        {
            for (int i = 0; i < input.Length; i++)
            {
                input[i] = Process(input[i]);
            }
        }

        /// 

        /// Calculates all coefficients.
        /// 

        protected abstract void CalculateBiQuadCoefficients();
    }

    /// 

    /// Used to apply a lowpass-filter to a signal.
    /// 

    public class LowpassFilter : BiQuad
    {
        /// 

        /// Initializes a new instance of the  class.
        /// 

        /// The sample rate.
        /// The filter's corner frequency.
        public LowpassFilter(int sampleRate, double frequency)
            : base(sampleRate, frequency)
        {
        }

        /// 

        /// Calculates all coefficients.
        /// 

        protected override void CalculateBiQuadCoefficients()
        {
            double k = Math.Tan(Math.PI * Frequency / SampleRate);
            var norm = 1 / (1 + k / Q + k * k);
            A0 = k * k * norm;
            A1 = 2 * A0;
            A2 = A0;
            B1 = 2 * (k * k - 1) * norm;
            B2 = (1 - k / Q + k * k) * norm;
        }
    }

    /// 

    /// Used to apply a highpass-filter to a signal.
    /// 

    public class HighpassFilter : BiQuad
    {
        //private int p1;
        //private double p2;

        /// 

        /// Initializes a new instance of the  class.
        /// 

        /// The sample rate.
        /// The filter's corner frequency.
        public HighpassFilter(int sampleRate, double frequency)
            : base(sampleRate, frequency)
        {
        }

        /// 

        /// Calculates all coefficients.
        /// 

        protected override void CalculateBiQuadCoefficients()
        {
            double k = Math.Tan(Math.PI * Frequency / SampleRate);
            var norm = 1 / (1 + k / Q + k * k);
            A0 = 1 * norm;
            A1 = -2 * A0;
            A2 = A0;
            B1 = 2 * (k * k - 1) * norm;
            B2 = (1 - k / Q + k * k) * norm;
        }
    }

    /// 

    /// Used to apply a bandpass-filter to a signal.
    /// 

    public class BandpassFilter : BiQuad
    {
        /// 

        /// Initializes a new instance of the  class.
        /// 

        /// The sample rate.
        /// The filter's corner frequency.
        public BandpassFilter(int sampleRate, double frequency)
            : base(sampleRate, frequency)
        {
        }

        /// 

        /// Calculates all coefficients.
        /// 

        protected override void CalculateBiQuadCoefficients()
        {
            double k = Math.Tan(Math.PI * Frequency / SampleRate);
            double norm = 1 / (1 + k / Q + k * k);
            A0 = k / Q * norm;
            A1 = 0;
            A2 = -A0;
            B1 = 2 * (k * k - 1) * norm;
            B2 = (1 - k / Q + k * k) * norm;
        }
    }

    /// 

    /// Used to apply a notch-filter to a signal.
    /// 

    public class NotchFilter : BiQuad
    {
        /// 

        /// Initializes a new instance of the  class.
        /// 

        /// The sample rate.
        /// The filter's corner frequency.
        public NotchFilter(int sampleRate, double frequency)
            : base(sampleRate, frequency)
        {
        }

        /// 

        /// Calculates all coefficients.
        /// 

        protected override void CalculateBiQuadCoefficients()
        {
            double k = Math.Tan(Math.PI * Frequency / SampleRate);
            double norm = 1 / (1 + k / Q + k * k);
            A0 = (1 + k * k) * norm;
            A1 = 2 * (k * k - 1) * norm;
            A2 = A0;
            B1 = A1;
            B2 = (1 - k / Q + k * k) * norm;
        }
    }

    /// 

    /// Used to apply a lowshelf-filter to a signal.
    /// 

    public class LowShelfFilter : BiQuad
    {
        /// 

        /// Initializes a new instance of the  class.
        /// 

        /// The sample rate.
        /// The filter's corner frequency.
        /// Gain value in dB.
        public LowShelfFilter(int sampleRate, double frequency, double gainDB)
            : base(sampleRate, frequency)
        {
            GainDB = gainDB;
        }

        /// 

        /// Calculates all coefficients.
        /// 

        protected override void CalculateBiQuadCoefficients()
        {
            const double sqrt2 = 1.4142135623730951;
            double k = Math.Tan(Math.PI * Frequency / SampleRate);
            double v = Math.Pow(10, Math.Abs(GainDB) / 20.0);
            double norm;
            if (GainDB >= 0)
            {   //boost
                norm = 1 / (1 + sqrt2 * k + k * k);
                A0 = (1 + Math.Sqrt(2 * v) * k + v * k * k) * norm;
                A1 = 2 * (v * k * k - 1) * norm;
                A2 = (1 - Math.Sqrt(2 * v) * k + v * k * k) * norm;
                B1 = 2 * (k * k - 1) * norm;
                B2 = (1 - sqrt2 * k + k * k) * norm;
            }
            else
            {   //cut
                norm = 1 / (1 + Math.Sqrt(2 * v) * k + v * k * k);
                A0 = (1 + sqrt2 * k + k * k) * norm;
                A1 = 2 * (k * k - 1) * norm;
                A2 = (1 - sqrt2 * k + k * k) * norm;
                B1 = 2 * (v * k * k - 1) * norm;
                B2 = (1 - Math.Sqrt(2 * v) * k + v * k * k) * norm;
            }
        }
    }

    /// 

    /// Used to apply a highshelf-filter to a signal.
    /// 

    public class HighShelfFilter : BiQuad
    {
        /// 

        /// Initializes a new instance of the  class.
        /// 

        /// The sample rate.
        /// The filter's corner frequency.
        /// Gain value in dB.
        public HighShelfFilter(int sampleRate, double frequency, double gainDB)
            : base(sampleRate, frequency)
        {
            GainDB = gainDB;
        }

        /// 

        /// Calculates all coefficients.
        /// 

        protected override void CalculateBiQuadCoefficients()
        {
            const double sqrt2 = 1.4142135623730951;
            double k = Math.Tan(Math.PI * Frequency / SampleRate);
            double v = Math.Pow(10, Math.Abs(GainDB) / 20.0);
            double norm;
            if (GainDB >= 0)
            {   //boost
                norm = 1 / (1 + sqrt2 * k + k * k);
                A0 = (v + Math.Sqrt(2 * v) * k + k * k) * norm;
                A1 = 2 * (k * k - v) * norm;
                A2 = (v - Math.Sqrt(2 * v) * k + k * k) * norm;
                B1 = 2 * (k * k - 1) * norm;
                B2 = (1 - sqrt2 * k + k * k) * norm;
            }
            else
            {   //cut
                norm = 1 / (v + Math.Sqrt(2 * v) * k + k * k);
                A0 = (1 + sqrt2 * k + k * k) * norm;
                A1 = 2 * (k * k - 1) * norm;
                A2 = (1 - sqrt2 * k + k * k) * norm;
                B1 = 2 * (k * k - v) * norm;
                B2 = (v - Math.Sqrt(2 * v) * k + k * k) * norm;
            }
        }
    }

    /// 

    /// Used to apply an peak-filter to a signal.
    /// 

    public class PeakFilter : BiQuad
    {
        /// 

        /// Gets or sets the bandwidth.
        /// 

        public double BandWidth
        {
            get { return Q; }
            set
            {
                if (value <= 0)
                    throw new ArgumentOutOfRangeException("value");
                Q = value;
            }
        }

        /// 

        /// Initializes a new instance of the  class.
        /// 

        /// The sampleRate of the audio data to process.
        /// The center frequency to adjust.
        /// The bandWidth.
        /// The gain value in dB.
        public PeakFilter(int sampleRate, double frequency, double bandWidth, double peakGainDB)
            : base(sampleRate, frequency, bandWidth)
        {
            GainDB = peakGainDB;
        }

        /// 

        /// Calculates all coefficients.
        /// 

        protected override void CalculateBiQuadCoefficients()
        {
            double norm;
            double v = Math.Pow(10, Math.Abs(GainDB) / 20.0);
            double k = Math.Tan(Math.PI * Frequency / SampleRate);
            double q = Q;

            if (GainDB >= 0) //boost
            {
                norm = 1 / (1 + 1 / q * k + k * k);
                A0 = (1 + v / q * k + k * k) * norm;
                A1 = 2 * (k * k - 1) * norm;
                A2 = (1 - v / q * k + k * k) * norm;
                B1 = A1;
                B2 = (1 - 1 / q * k + k * k) * norm;
            }
            else //cut
            {
                norm = 1 / (1 + v / q * k + k * k);
                A0 = (1 + 1 / q * k + k * k) * norm;
                A1 = 2 * (k * k - 1) * norm;
                A2 = (1 - 1 / q * k + k * k) * norm;
                B1 = A1;
                B2 = (1 - v / q * k + k * k) * norm;
            }
        }
    }
}

namespace GarantEEG
{
    public interface IFilter
    {
        /**
         * @brief Type Получить тип фильтра
         * @return Тип фильтра
         */
        int Type();

        /**
         * @brief Rate Получить рабочую частоту фильтра
         * @return Частота
         */
        int Rate();

        /**
         * @brief Frequency Получить частоту среза
         * @return Частота
         */
        int Frequency();

        /**
		 * @brief Process Функция фильтрации данных
		 * @param input Данные
		 */
        void Process(float[] input);
    }
}
