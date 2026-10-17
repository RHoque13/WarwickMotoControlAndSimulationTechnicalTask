files = dir('*.tdms');

totalData = [];
medianArray = [];

for i = 1:length(files)
    currentData = getData(files(i).name);
    totalData = [totalData; currentData];
    medianVal = median(currentData); 
    medianArray = [medianArray; medianVal];
end


function processData = getData(filename)

    data = TDMS_getStruct(filename);
    torqueData = data.g_100Hz.FMI_1.data;
    speedData = data.g_100Hz.SpeedMeasured.data;
    mechanicalPowerData = data.g_100Hz.Mechanical_Power.data;
    electricalPowerData = data.g_100Hz.EV_Measured_Power.data;
    
    idxStart = find(torqueData >0,1,"first");
    idxLast = find(torqueData >0,1,'last');
    
    idxRange = idxStart:idxLast;
    torqueSegment = torqueData(idxRange);
    speedSegment = speedData(idxRange);
    mechanicalPowerSegment = mechanicalPowerData(idxRange);
    electricalPowerSegment = electricalPowerData(idxRange);
    
    dataArray = [torqueSegment;speedSegment;mechanicalPowerSegment;electricalPowerSegment]';
    
    % processData = dataArray;
    
    maxTorqueData = zeros(0,4);

    for i = 1:size(dataArray,1)

        currentSpeed = dataArray(i,2);
        currentTorque = dataArray(i,1);

        index = find(abs(maxTorqueData(:,2) - currentSpeed) < 0.5, 1);

        if isempty(index)
            maxTorqueData(end+1,:) = dataArray(i,:);

        elseif currentTorque > maxTorqueData(index,1)
            maxTorqueData(index,:) = dataArray(i,:);
        end

    end

    maxTorqueData = sortrows(maxTorqueData,2);

    outliers = isoutlier(maxTorqueData(:,1),"movmedian",7,"ThresholdFactor",1.);
    maxTorqueData(outliers,:) = [];

    maxTorqueData(:,3) = smoothdata(maxTorqueData(:,3), "movmedian",7);
    maxTorqueData = maxTorqueData(4:end,:);

    average = median(maxTorqueData(:,1));

    processData = maxTorqueData;

    
end

correlationMatrix = corrcoef(totalData(:,1), totalData(:,2));
correlationVal = correlationMatrix(1,2);

indexOfSpace = find(diff(totalData(:,2)) > 50);

for i =1:length(indexOfSpace)

    tempData1 = totalData(indexOfSpace(i),:);
    tempData2 = totalData(indexOfSpace(i) + 1,:);

    diffRPM = tempData2(:,2) - tempData1(:,2);

    numOfPointsToAdd = round(diffRPM / 20);
    interpolatedDataTorque = [linspace(tempData1(1), tempData2(1), numOfPointsToAdd)];
    interpolatedDataSpeed = [linspace(tempData1(2), tempData2(2), numOfPointsToAdd)];
    interpolatedDataMPower = [linspace(tempData1(3), tempData2(3), numOfPointsToAdd)];
    interpolatedDataEPower = [linspace(tempData1(4), tempData2(4), numOfPointsToAdd)];

    interpolatedData = [interpolatedDataTorque; interpolatedDataSpeed; interpolatedDataMPower; interpolatedDataEPower]';

    totalData = [totalData; interpolatedData];

end


totalData = sortrows(totalData, 2);

totalData(:,1) = smoothdata(totalData(:,1), "movmedian",21);

totalData(:,3) = smoothdata(totalData(:,3), "movmedian",21);

outliers = isoutlier(totalData(:,1),"movmedian",7,"ThresholdFactor",1);
totalData(outliers,:) = [];

fittedTorque = [totalData(1,1);totalData(:,1)];
fittedRPM =[0;totalData(:,2)];

figure;

yyaxis left
plot(fittedRPM, fittedTorque, 'LineWidth',1.0,'Color', 'r');
xlabel('Speed (RPM)');
ylabel('Torque');
ylim([0 inf]);
xlim([0 inf]);

gradientArray = [];

for i = 1:(length(medianArray)-1)

    torqueDiff = medianArray(i+1,1) - medianArray(i,1);
    speedDiff = medianArray(i+1,2) - medianArray(i,2);
    gradientVal = torqueDiff / speedDiff;
    gradientArray = [gradientArray; gradientVal];

end

calculatedGradientLinearRegion = mean(gradientArray);

fprintf('The gradient of the linear region is %.6f\n', calculatedGradientLinearRegion);
fprintf('The correlation of the data is %.6f\n', correlationVal);



% yyaxis right
% plot(totalData(:,2), totalData(:,3), 'LineWidth',1.0, 'Color', 'b');
% hold on
% ylabel('Mechanical Power');
% ylim([0 inf]);



% figure;
% plot(totalData(:,2),totalData(:,4), Color='g');



% totalData = sortrows(totalData,1);
% 
% [upper,lower] = envelope(totalData(:,1), length(totalData), 'rms');
% x = totalData(:,2);
% 
% plot(upper, Color='r')
% hold on
% % plot(lower, Color='b')

