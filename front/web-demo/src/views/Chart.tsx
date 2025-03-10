import {
  Chart as ChartJS,
  CategoryScale,
  LinearScale,
  PointElement,
  LineElement,
  Title,
  Tooltip,
  Legend,
  elements,
} from "chart.js";
import { useState, useEffect } from "react";
import { Line } from "react-chartjs-2";
import axios from "axios";

ChartJS.register(
  CategoryScale,
  LinearScale,
  PointElement,
  LineElement,
  Title,
  Tooltip,
  Legend,
  elements
);

const ChartComponent = () => {
  const [chartValues, setChartValues] = useState<number[]>([8, 2, 5, 9, 5, 11, 3, 5, 10, 0, 1, 8, 2, 9, 0, 13, 10, 7, 16]);

  useEffect(() => {
    const fetchData = async () => {
      try {
        const response = await axios.get("/api/v1/temp/raw");
        setChartValues((prev) => [...prev.slice(1), response.data.raw]);
      } catch (error) {
        console.error(error);
      }
    };

    const interval = setInterval(fetchData, 1000); // Fetch data every 1 second
    return () => clearInterval(interval);
  }, []);

  const options = {
    responsive: true,
    maintainAspectRatio: false,
    plugins: {
      legend: {
        position: "top" as const,
      },
      title: {
        display: true,
        text: "Dynamic Chart",
      },
    },
    elements: {
      line: {
        tension: 0.5,
      },
    },
  };

  const data = {
    labels: Array.from({ length: chartValues.length }, (_, i) => i + 1),
    datasets: [
      {
        label: "Chart Data",
        data: chartValues,
        // fill: false,
        borderColor: "rgb(75, 192, 192)",
        backgroundColor: "hsla(347, 100.00%, 69.40%, 0.50)",
      },
    ],
  };

  return <Line options={options} data={data} />;
};

const Chart = () => {
  return (
    <div>
      <div className="flex justify-center items-center mt-25">
        <div className="card bg-base-100 h-1/2 w-1/2 shadow-lg">
          <ChartComponent />
        </div>
      </div>
    </div>
  );
};

export default Chart;

